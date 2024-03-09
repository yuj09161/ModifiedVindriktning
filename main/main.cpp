#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <climits>
#include <cfloat>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "esp_check.h"
#include "esp_log.h"

#include "config.h"
#include "wifi/wifi.h"
#include "smartthings/request.h"
#include "io.h"
#include "colors.h"
#include "sample_array.h"
#include "semaphore.h"

#if defined(CONFIG_HEAP_TRACING) && CONFIG_LOG_DEFAULT_LEVEL >= 4  // When default log level is verbose than DEBUG
#include "esp_heap_trace.h"

#define NUM_RECORDS 500
static heap_trace_record_t trace_record[NUM_RECORDS];
#endif


#define DEVICE_STATUS_TASK_STARTED (1 << 0)
#define GET_SENSOR_VALUE_TASK_STARTED (1 << 1)
#define WS2812_HELLO_TASK_ENDED (1 << 2)
#define ALL_TASK_STARTED (\
    DEVICE_STATUS_TASK_STARTED\
    | GET_SENSOR_VALUE_TASK_STARTED\
)
#define IS_ALL_TASK_STARTED ((taskStatusFlags & ALL_TASK_STARTED) == ALL_TASK_STARTED)
#define IS_WS2812_HELLO_TASK_ENDED ((taskStatusFlags & WS2812_HELLO_TASK_ENDED) == WS2812_HELLO_TASK_ENDED)


static IntSampleArray
    *fine_dust_array,
    *humidity_array,
    *tvoc_array;
static SampleArray
    *temperature_array,
    *temperature2_array,
    *pressure_array;
static DeviceConfig deviceConfig;
static SemaphoreHandle_t configSemaphore, lastAverageSemaphore;
static sensor_values lastAverage;
static uint_fast8_t taskStatusFlags, isSensorInitFailed;
static TickType_t fanStartedTime;

PWMLed *statusLED;
WS2812Strip *strip;



void init_variables(void) {
    configSemaphore = xSemaphoreCreateCounting(CONFIG_SEMAPHORE_MAX_VALUE, CONFIG_SEMAPHORE_MAX_VALUE);
    if (configSemaphore == NULL) {
        ESP_LOGE("Main:init_variables", "Failed to create configReadSemaphore. Rebooting...");
        abort();
    }
    lastAverageSemaphore = xSemaphoreCreateCounting(CONFIG_SEMAPHORE_MAX_VALUE, CONFIG_SEMAPHORE_MAX_VALUE);
    if (lastAverageSemaphore == NULL) {
        ESP_LOGE("Main:init_variables", "Failed to create lastAverageReadSemaphore. Rebooting...");
        abort();
    }

    // Set default config
    deviceConfig.tempHigh        = 27;
    deviceConfig.tempLow         = 18;
    deviceConfig.humiHigh        = 60;
    deviceConfig.humiLow         = 40;
    deviceConfig.fineDustVeryBad = 150;
    deviceConfig.fineDustBad     = 100;
    deviceConfig.fineDustWarning = 50;
    deviceConfig.fineDustNormal  = 15;
    deviceConfig.illuminanceHigh = 5;
    deviceConfig.illuminanceLow  = 3;
    deviceConfig.offsets.temperature  = 0;
    deviceConfig.offsets.humidity     = 0;
    deviceConfig.offsets.tvoc         = 0;
    deviceConfig.offsets.temperature2 = 0;
    deviceConfig.offsets.pressure     = 0;

    fine_dust_array    = new IntSampleArray(SAMPLE_PER_UPDATE_STATUS);
    temperature_array  = new SampleArray(SAMPLE_PER_UPDATE_STATUS);
    humidity_array     = new IntSampleArray(SAMPLE_PER_UPDATE_STATUS);
    temperature2_array = new SampleArray(SAMPLE_PER_UPDATE_STATUS);
    pressure_array     = new SampleArray(SAMPLE_PER_UPDATE_STATUS);
    tvoc_array         = new IntSampleArray(SAMPLE_PER_UPDATE_STATUS);
}

void get_device_status() {
    STStatus status;
    uint_fast8_t bright;
    static uint_fast8_t lastFanState = 1;

    // Front WS2812 bright
    if (get_device_status(&status) != ESP_OK) {
        ESP_LOGE("Main:get_device_status", "Failed to get status.");
        ESP_ERROR_CHECK(strip->setColor(BOTTOM_LED, WS2812_RED));
        return;
    }
    ESP_ERROR_CHECK(strip->setColor(BOTTOM_LED, WS2812_OFF));


    if (!xSemaphoreTake(configSemaphore, SEMAPHORE_MAX_WAIT)) {
        ESP_LOGE("Main:get_device_status", "configSemaphore not released after SEMAPHORE_MAX_WAIT. Rebooting...");
        abort();
    }

    ESP_LOGI("Main:get_device_status", "switchLevel: %d, fanSpeed: %d", status.switchLevel, status.fanSpeed);

    if (status.switchLevel >= 70)
        bright = deviceConfig.illuminanceHigh;
    else if (status.switchLevel >= 30)
        bright = deviceConfig.illuminanceLow;
    else
        bright = 0;

    xSemaphoreGive(configSemaphore);

    ESP_ERROR_CHECK(strip->setBright(bright));

    if (unlikely(lastFanState != !!status.fanSpeed)) {
        if (status.fanSpeed) {
            if (likely(fan_on() == ESP_OK))
                fanStartedTime = xTaskGetTickCount();
            else
                ESP_LOGE("Main:get_device_status", "Failed to turn on fan.");
        } else {
            if (likely(fan_off() == ESP_OK))
                fanStartedTime = portMAX_DELAY;
            else
                ESP_LOGE("Main:get_device_status", "Failed to turn off fan.");
        }
        lastFanState = !!status.fanSpeed;
    }
}

void get_device_config() {
    DeviceConfig newConfig;
    if (get_device_config(&newConfig) != ESP_OK) {
        ESP_LOGE("Main:get_device_config", "Failed to get config.");
        ESP_ERROR_CHECK(strip->setColor(BOTTOM_LED, WS2812_RED));
        return;
    }
    ESP_ERROR_CHECK(strip->setColor(BOTTOM_LED, WS2812_OFF));
    if (!xSemaphoreTakeN(configSemaphore, CONFIG_SEMAPHORE_MAX_VALUE, SEMAPHORE_MAX_WAIT)) {
        ESP_LOGE("Main:get_device_config", "configSemaphore not released after SEMAPHORE_MAX_WAIT. Rebooting...");
        abort();
    }
    deviceConfig = newConfig;
    xSemaphoreGiveN(configSemaphore, CONFIG_SEMAPHORE_MAX_VALUE);

    if (deviceConfig.offsets.temperature > 100 || deviceConfig.offsets.temperature < -100)
        abort();
    if (deviceConfig.offsets.temperature2 > 100 || deviceConfig.offsets.temperature2 < -100)
        abort();

    temperature_array->setOffset(deviceConfig.offsets.temperature);
    humidity_array->setOffset(deviceConfig.offsets.humidity);
    tvoc_array->setOffset(deviceConfig.offsets.tvoc);
    temperature2_array->setOffset(deviceConfig.offsets.temperature2);
    pressure_array->setOffset(deviceConfig.offsets.pressure);
}

void update_device_status() {
    ESP_LOGD("Main:update_device_status", "Update start...");

    if (!xSemaphoreTake(lastAverageSemaphore, SEMAPHORE_MAX_WAIT)) {
        ESP_LOGE("Main:update_device_status", "lastAverageSemaphore not released after SEMAPHORE_MAX_WAIT. Rebooting...");
        abort();
    }
    sensor_values average = lastAverage;
    xSemaphoreGive(lastAverageSemaphore);

    if (set_device_status(
        average.fine_dust,
        average.temperature,
        average.humidity,
        average.tvoc,
        average.temperature2,
        average.pressure / 10.0f  // hPa to kPa
    ) == ESP_OK) {
        ESP_LOGI("Main:update_device_status", "Successfully update current status.");
        ESP_ERROR_CHECK(strip->setColor(BOTTOM_LED, WS2812_OFF));
    } else {
        ESP_LOGE("Main:update_device_status", "Failed to update current status.");
        ESP_ERROR_CHECK(strip->setColor(BOTTOM_LED, WS2812_RED));
    };

    ESP_LOGD("Main:update_device_status", "Update done.");
}


void set_ws2812_color() {
    ESP_LOGD("Main:set_ws2812_color", "Start set...");

    if (!xSemaphoreTake(lastAverageSemaphore, SEMAPHORE_MAX_WAIT)) {
        ESP_LOGE("Main:set_ws2812_color", "lastAverageSemaphore not released after SEMAPHORE_MAX_WAIT. Rebooting...");
        abort();
    }
    sensor_values average = lastAverage;
    xSemaphoreGive(lastAverageSemaphore);
    if (!xSemaphoreTake(configSemaphore, SEMAPHORE_MAX_WAIT)) {
        ESP_LOGE("Main:set_ws2812_color", "configSemaphore not released after SEMAPHORE_MAX_WAIT. Rebooting...");
        abort();
    }
    DeviceConfig config = deviceConfig;
    xSemaphoreGive(configSemaphore);

    if (unlikely(average.temperature) == FLT_MIN) {
        ESP_LOGI("Main:set_ws2812_color", "Temperature is invalid. Turn off pixel...");
        ESP_ERROR_CHECK(strip->setColor(TEMPERATURE_LED, WS2812_OFF, false));
    } else if (average.temperature > config.tempHigh)
        ESP_ERROR_CHECK(strip->setColor(TEMPERATURE_LED, WS2812_RED, false));
    else if (average.temperature >= config.tempLow)
        ESP_ERROR_CHECK(strip->setColor(TEMPERATURE_LED, WS2812_GREEN, false));
    else
        ESP_ERROR_CHECK(strip->setColor(TEMPERATURE_LED, WS2812_BLUE, false));

    if (unlikely(average.humidity) == INT_MIN) {
        ESP_LOGI("Main:set_ws2812_color", "Humidity is invalid. Turn off pixel...");
        ESP_ERROR_CHECK(strip->setColor(HUMIDITY_LED, WS2812_OFF, false));
    } else if (average.humidity > config.humiHigh)
        ESP_ERROR_CHECK(strip->setColor(HUMIDITY_LED, WS2812_RED, false));
    else if (average.humidity >= config.humiLow)
        ESP_ERROR_CHECK(strip->setColor(HUMIDITY_LED, WS2812_GREEN, false));
    else
        ESP_ERROR_CHECK(strip->setColor(HUMIDITY_LED, WS2812_BLUE, false));

    if (unlikely(average.fine_dust) == INT_MIN) {
        ESP_LOGI("Main:set_ws2812_color", "PM1006 value is invalid. Turn off pixel...");
        ESP_ERROR_CHECK(strip->setColor(FINEDUST_LED, WS2812_OFF, false));
    } else if (unlikely(average.fine_dust > PM1006_THRESHOLD))
        ESP_ERROR_CHECK(strip->setColor(FINEDUST_LED, WS2812_WHITE, false));
    else if (average.fine_dust >= config.fineDustVeryBad)
        ESP_ERROR_CHECK(strip->setColor(FINEDUST_LED, WS2812_RED, false));
    else if (average.fine_dust >= config.fineDustBad)
        ESP_ERROR_CHECK(strip->setColor(FINEDUST_LED, WS2812_ORANGE, false));
    else if (average.fine_dust >= config.fineDustWarning)
        ESP_ERROR_CHECK(strip->setColor(FINEDUST_LED, WS2812_YELLOW, false));
    else if (average.fine_dust >= config.fineDustNormal)
        ESP_ERROR_CHECK(strip->setColor(FINEDUST_LED, WS2812_GREEN, false));
    else
        ESP_ERROR_CHECK(strip->setColor(FINEDUST_LED, WS2812_BLUE, false));

    ESP_ERROR_CHECK(strip->refresh());

    ESP_LOGD("Main:set_ws2812_color", "Done...");
}

void led_task(void *) {
    while (1) {
        statusLED->toggle();
        if (unlikely(isSensorInitFailed))
            vTaskDelay(pdMS_TO_TICKS(500));
        else
            vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void ws2812_hello_task(void *) {
    // Until another task started
    while (!IS_ALL_TASK_STARTED && !isSensorInitFailed) {
        for (int i = 0; i < 7 && !IS_ALL_TASK_STARTED && !isSensorInitFailed; i++) {
            ESP_ERROR_CHECK(strip->setColor(i, WS2812_BLUE));
            vTaskDelay(pdMS_TO_TICKS(100));
            ESP_ERROR_CHECK(strip->setColor(i, WS2812_OFF, false));
        }
    }

    if (isSensorInitFailed)
        while (1) {  // Display error
            for (int i = 0; i < 7; i++)
                ESP_ERROR_CHECK(strip->setColor(i, WS2812_RED));
            vTaskDelay(pdMS_TO_TICKS(500));
            for (int i = 0; i < 7; i++)
                ESP_ERROR_CHECK(strip->setColor(i, WS2812_OFF));
            vTaskDelay(pdMS_TO_TICKS(500));
        }

    set_ws2812_color();
    taskStatusFlags |= WS2812_HELLO_TASK_ENDED;
    vTaskDelete(NULL);
}

void get_sensor_value_task(void *sensorStartupTickPtrV) {
    TickType_t tmpTick;
    sensor_values values;
    int startupCnt = SAMPLE_PER_UPDATE_STATUS;

    // Wait until sensor prepaired
    TickType_t sensorStartupTick = *((TickType_t*)sensorStartupTickPtrV);
    ESP_LOGI("Main:get_sensor_value_task", "Wait for sensors init.");
    vTaskDelayUntil(&sensorStartupTick, pdMS_TO_TICKS(SENSOR_STARTUP_DELAY));

    // Wait until PM1006 reading valid
    ESP_LOGI("Main:get_sensor_value_task", "Wait for PM1006 fan.");
    tmpTick = fanStartedTime;
    vTaskDelayUntil(&tmpTick, pdMS_TO_TICKS(PM1006_FAN_STARTUP_DELAY));

    TickType_t lastTick = xTaskGetTickCount();
    while (1) {
        ESP_LOGD("Main:get_sensor_value_task", "Start update sensor...");

        // Read sensor values
        get_sensor_values(&values);
        // Write values to average arrays
        tmpTick = xTaskGetTickCount();
        if (
            tmpTick < fanStartedTime
            || tmpTick - fanStartedTime < pdMS_TO_TICKS(PM1006_FAN_STARTUP_DELAY)
            || values.fine_dust == INT_MIN
        )  // Reading of PM1006 is invalid when fan is turned off or on just now
            fine_dust_array->invalidate();
        else if (values.fine_dust > PM1006_THRESHOLD) {
            ESP_LOGI("Main:set_ws2812_color", "Too high PM1006 value (%dµg/m^3) detected.", values.fine_dust);
            fine_dust_array->writeValue(INT_MAX);
        } else
            fine_dust_array->writeValue(values.fine_dust);
        if (values.temperature == FLT_MIN)
            temperature_array->invalidate();
        else
            temperature_array->writeValue(values.temperature);
        if (values.humidity == INT_MIN)
            humidity_array->invalidate();
        else
            humidity_array->writeValue(values.humidity);
        if (values.temperature2 == FLT_MIN)
            temperature2_array->invalidate();
        else
            temperature2_array->writeValue(values.temperature2);
        if (values.pressure == FLT_MIN)
            pressure_array->invalidate();
        else
            pressure_array->writeValue(values.pressure);
        if (values.tvoc == INT_MIN)
            tvoc_array->invalidate();
        else
            tvoc_array->writeValue(values.tvoc);

        switch (startupCnt) {
            case 0:
            case 1:
                if (!xSemaphoreTakeN(lastAverageSemaphore, LAST_AVERAGE_SEMAPHORE_MAX_VALUE, SEMAPHORE_MAX_WAIT)) {
                    ESP_LOGE("Main:get_sensor_value_task", "lastAverageSemaphore not released after SEMAPHORE_MAX_WAIT. Rebooting...", );
                    abort();
                }
                // Calculate new average
                lastAverage.fine_dust    = fine_dust_array->getAverage();
                lastAverage.temperature  = temperature_array->getAverage();
                lastAverage.humidity     = humidity_array->getAverage();
                lastAverage.temperature2 = temperature2_array->getAverage();
                lastAverage.pressure     = pressure_array->getAverage();
                lastAverage.tvoc         = tvoc_array->getAverage();
                xSemaphoreGiveN(lastAverageSemaphore, LAST_AVERAGE_SEMAPHORE_MAX_VALUE);

                // Update WS2812 Color
                if (likely(IS_WS2812_HELLO_TASK_ENDED)) {
                    ESP_LOGD("Main:get_sensor_value_task", "Update WS2812...");
                    set_ws2812_color();
                } else {
                    ESP_LOGD("Main:get_sensor_value_task", "Skipping update WS2812...");
                }

                ESP_LOGD("Main:get_sensor_value_task", "Done update sensor.");
                vTaskDelayUntil(&lastTick, pdMS_TO_TICKS(GET_SENSOR_TASK_DELAY));
                break;
            case 2:
            case 3:
                startupCnt--;
                ESP_LOGI("Main:get_sensor_value_task", "(Startup) Wait for sensors ready for next reading.");
                vTaskDelayUntil(&lastTick, pdMS_TO_TICKS(GET_SENSOR_TASK_DELAY_MIN));
                break;
            default:
                abort();
        }
        if (startupCnt == 1) {
            startupCnt--;
            taskStatusFlags |= GET_SENSOR_VALUE_TASK_STARTED;
        }
    }
}

void device_status_task(void *) {
    get_device_config();
    get_device_status();
    while (!(taskStatusFlags & GET_SENSOR_VALUE_TASK_STARTED))
        vTaskDelay(pdMS_TO_TICKS(250));
    taskStatusFlags |= DEVICE_STATUS_TASK_STARTED;
    update_device_status();

    TickType_t lastTick = xTaskGetTickCount();
    for (unsigned int i = 0; ; i++) {
        ESP_LOGI("device_status_task", "==========Free Mem: %u==========", heap_caps_get_free_size(MALLOC_CAP_DEFAULT));
        get_device_status();
        if (!(i % GET_CONFIG_PER_GET_STATUS))
            get_device_config();
        if (!(i % UPDATE_STATUS_PER_GET_STATUS))
            update_device_status();
        vTaskDelayUntil(&lastTick, pdMS_TO_TICKS(GET_STATUS_INTERVAL));
    }
}

void monitor_wifi_task(void *) {
    while (1) {
        chech_and_reconnect_wifi();
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

extern "C" void app_main(void) {
    init_gpio();
    fanStartedTime = xTaskGetTickCount();

    init_modules(&statusLED, &strip);
    xTaskCreate(led_task, "led_task", 2048, NULL, 10, NULL);
    xTaskCreate(ws2812_hello_task, "ws2812_hello_task", 4096, NULL, 10, NULL);
    if (init_sensors() != ESP_OK) {
        // Display error, and stop futher operation
        isSensorInitFailed = 1;
        return;
    }
    init_sensors();
    TickType_t sensorStartupTime = xTaskGetTickCount();
    init_wifi();
    init_variables();

#if defined(CONFIG_HEAP_TRACING) && CONFIG_LOG_DEFAULT_LEVEL >= 4  // When default log level is verbose than DEBUG
    ESP_ERROR_CHECK(heap_trace_init_standalone(trace_record, NUM_RECORDS));
#endif

    xTaskCreate(get_sensor_value_task, "get_sensor_value_task", 4096, &sensorStartupTime, 12, NULL);
    xTaskCreate(device_status_task, "device_status_task", 4096, NULL, 14, NULL);
    xTaskCreate(monitor_wifi_task, "monitor_wifi_task", 4096, NULL, 16, NULL);
}