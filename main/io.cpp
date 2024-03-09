#include <cstdint>
#include <cstring>
#include <climits>
#include <cfloat>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "driver/uart.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include "esp_check.h"
#include "esp_adc/adc_oneshot.h"

#include "led_strip.h"

#include "config.h"
#include "io.h"

#include "pwm_led.h"
#include "ws2812.h"

#include "pm1006.h"
#include "aht20.h"
#include "bmp280.h"
#include "ags02ma.h"

#define BUF_SIZE 50

#define STATUS_LED_LEDC_CHANNEL LEDC_CHANNEL_0
#define FAN_LEDC_CHANNEL LEDC_CHANNEL_1


aht20_dev_handle_t aht20;
led_strip_handle_t ws2812;
pm1006_handle_t    pm1006;

AGS02MA *ags02ma;
BMP280  *bmp280;

esp_err_t get_pm1006_value(int *fine_dust);
esp_err_t get_temphumi(float *temperature, int *humidity);
esp_err_t get_temppress(float *temperature, float *pressure);
esp_err_t get_tvoc(int *tvoc);


void init_gpio(void) {
    // PM1006 Fan
    gpio_config_t cfg = {
        .pin_bit_mask = 1ll << PIN_PM1006_FAN,
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE
    };
    gpio_config(&cfg);
    gpio_set_level(PIN_PM1006_FAN, 1);
}

void init_modules(PWMLed **statusLEDPtr, WS2812Strip **stripPtr) {
    // Status LED - LEDC
    PWMLed *statusLED;
    ESP_LOGI("IO:init_modules", "Initializing onboard status LED...");
    statusLED = new PWMLed(PIN_LED, LEDC_TIMER_0, LEDC_CHANNEL_0);
    ESP_ERROR_CHECK(statusLED->begin());
    statusLED->setBright(STATUS_LED_BRIGHT);
    *statusLEDPtr = statusLED;

    // WS2812 - espressif/led_strip (RMT, with Wrapper class)
    WS2812Strip *strip;
    ESP_LOGI("IO:init_modules", "Initializing WS2812...");
    strip = new WS2812Strip(PIN_WS2812, 8);
    ESP_ERROR_CHECK(strip->begin());
    strip->setBright(WS2812_INIT_BRIGHT);
    *stripPtr = strip;
}

esp_err_t init_sensors(void) {
    // PM1006 - UART
    ESP_LOGI("IO:init_sensors", "Initializing PM1006...");
    pm1006_config_t pm1006_cfg = {};
        pm1006_cfg.uart_num    = UART_NUM_1;
        pm1006_cfg.tx_pin      = PIN_PM1006_TX;
        pm1006_cfg.rx_pin      = PIN_PM1006_RX;
        pm1006_cfg.tx_buf_size = PM1006_TX_BUF_SIZE;
        pm1006_cfg.rx_buf_size = PM1006_RX_BUF_SIZE;
        pm1006_cfg.timeout     = PM1006_TIMEOUT;
    ESP_RETURN_ON_ERROR(
        pm1006_create(&pm1006_cfg, &pm1006),
        "IO:init_sensors", "Failed to init PM1006."
    );

    // I2C Num0
    ESP_LOGI("IO:init_sensors", "Initializing I2C...");
    i2c_config_t i2c_num0_conf = {};
        i2c_num0_conf.mode             = I2C_MODE_MASTER;
        i2c_num0_conf.sda_io_num       = PIN_I2C_NUM0_SDA;
        i2c_num0_conf.scl_io_num       = PIN_I2C_NUM0_SCL;
        i2c_num0_conf.sda_pullup_en    = GPIO_PULLUP_DISABLE;
        i2c_num0_conf.scl_pullup_en    = GPIO_PULLUP_DISABLE;
        i2c_num0_conf.master.clk_speed = I2C_NUM0_CLOCK_SPEED;
    ESP_RETURN_ON_ERROR(
        i2c_param_config(I2C_NUM_0, &i2c_num0_conf),
        "IO:init_sensors", "Failed to init I2C."
    );
    ESP_RETURN_ON_ERROR(
        i2c_driver_install(I2C_NUM_0, i2c_num0_conf.mode, 0, 0, 0),
        "IO:init_sensors", "Failed to init I2C."
    );

#ifdef I2C_NUM1_CLOCK_SPEED
#error I2C NUM1 is not implemented.
#endif

    // AHT20 - espressif/aht20 (I2C)
    ESP_LOGI("IO:init_sensors", "Initializing AHT20...");
    aht20_i2c_config_t aht20_i2c_cfg = {
        .i2c_port = AHT20_I2C_NUM,
        .i2c_addr = (AHT20_I2C_ADDR << 1) // The library needs this
    };
    ESP_RETURN_ON_ERROR(
        aht20_new_sensor(&aht20_i2c_cfg, &aht20),
        "IO:init_sensors", "Failed to init AHT20."
    );

    // BMP280 - I2C
    ESP_LOGI("IO:init_sensors", "Initializing BMP280...");
    bmp280 = new BMP280(BMP280_I2C_NUM, BMP280_I2C_ADDR, BMP280_TIMEOUT);
    ESP_RETURN_ON_ERROR(
        bmp280->begin(BMP280::PM_NORMAL, BMP280::P_UHIGH, BMP280::T_STANDARD, BMP280::FILTER_OFF, BMP280::STBY_1s),
        "IO:init_sensors", "Failed to init BMP280."
    );

    // AGS02MA - I2C
    ESP_LOGI("IO:init_sensors", "Initializing AGS02MA...");
    ags02ma = new AGS02MA(AGS02MA_I2C_NUM, AGS02MA_I2C_ADDR, AGS02MA_TIMEOUT);
    ESP_RETURN_ON_ERROR(
        ags02ma->begin(),
        "IO:init_sensors", "Failed to init AGS02MA."
    );

    ESP_LOGI("IO:init_sensors", "Successfully initialized all sensors.");
    return ESP_OK;
}

esp_err_t get_sensor_values(sensor_values *result) {
    esp_err_t err;
    if ((err = get_pm1006_value(&result->fine_dust)) != ESP_OK) {
        ESP_LOGW(
            "IO:get_sensor_values:get_pm1006_value",
            "Failed to get PM1006 value (Error: %s).",
            esp_err_to_name(err)
        );
        result->fine_dust = INT_MIN;
    }
    if ((err = get_temphumi(&result->temperature, &result->humidity)) != ESP_OK) {
        ESP_LOGW(
            "IO:get_sensor_values:get_temphumi",
            "Failed to get temperature or humidity (Error: %s).",
            esp_err_to_name(err)
        );
        result->temperature = FLT_MIN;
        result->humidity    = INT_MIN;
    }
    if ((err = get_temppress(&result->temperature2, &result->pressure)) != ESP_OK) {
        ESP_LOGW(
            "IO:get_sensor_values:get_temppress",
            "Failed to get temperature or pressure (Error: %s).",
            esp_err_to_name(err)
        );
        result->temperature2 = FLT_MIN;
        result->pressure     = FLT_MIN;
    }
    if ((err = get_tvoc(&result->tvoc)) != ESP_OK) {
        ESP_LOGW(
            "IO:get_sensor_values:get_tvoc",
            "Failed to get TVOC (Error: %s).",
            esp_err_to_name(err)
        );
        result->tvoc = INT_MIN;
    }
    return ESP_OK;
}

esp_err_t get_pm1006_value(int *fine_dust) {
    ESP_RETURN_ON_ERROR(
        pm1006_get_value(pm1006, fine_dust),
        "IO:get_pm1006_value", "Failed to get PM2.5."
    );
    ESP_LOGI("IO:get_pm1006_value", "PM2.5: %dµg/m^3", *fine_dust);
    return ESP_OK;
}

esp_err_t get_temphumi(float *temperature, int *humidity) {
    uint32_t raw_temperature, raw_humidity;
    float decimal_temperature, decimal_humidity;
    ESP_RETURN_ON_ERROR(
        aht20_read_temperature_humidity(aht20, &raw_temperature, &decimal_temperature, &raw_humidity, &decimal_humidity),
        "IO:get_temphumi", "Failed to get temperature/humidity."
    );
    *temperature = decimal_temperature;
    *humidity = (int)(decimal_humidity);
    ESP_LOGI("IO:get_temphumi", "Temperature: %.01f°C | Humidity: %d%%", *temperature, *humidity);
    return ESP_OK;
}

esp_err_t get_temppress(float *temperature, float *pressure) {
    BMP280Result result = {};
    ESP_RETURN_ON_ERROR(
        bmp280->get(&result),
        "IO:get_temppress", "Failed to get temperature/air pressure."
    );
    *temperature = result.temperature;
    *pressure = result.pressure;
    ESP_LOGI("IO:get_temppress", "Temperature: %.01f°C | Air Pressure: %.02fhPa", *temperature, *pressure);
    return ESP_OK;
}

esp_err_t get_tvoc(int *tvoc) {
    int result;
    ESP_RETURN_ON_ERROR(
        ags02ma->get(&result),
        "IO:get_tvoc", "Failed to get TVOC."
    );
    *tvoc = result;
    ESP_LOGI("IO:get_tvoc", "TVOC: %dppb", *tvoc);
    return ESP_OK;
}

esp_err_t set_strip_pixels(led_pixel *pixels) {
    ESP_LOGD("IO:set_strip_pixels", "Starting set...");
    led_pixel pixel;
    int bright;
    for (int i = 0; i < 8; i++) {
        pixel = pixels[i];
        bright = 8 - pixel.bright;
        ESP_RETURN_ON_ERROR(led_strip_set_pixel(
            ws2812, i, pixel.r >> bright, pixel.g >> bright, pixel.b >> bright
        ), "set_strip_pixels", "");
    }
    ESP_RETURN_ON_ERROR(led_strip_refresh(ws2812), "set_strip_pixels", "Failed to set.");
    ESP_LOGD("IO:set_strip_pixels", "Done");
    return ESP_OK;
}

esp_err_t fan_off(void) {
    return gpio_set_level(PIN_PM1006_FAN, 0);
}

esp_err_t fan_on(void) {
    return gpio_set_level(PIN_PM1006_FAN, 1);
}
