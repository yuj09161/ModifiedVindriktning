#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "nvs_flash.h"
#include "esp_err.h"
#include "esp_check.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_log.h"

#include "config.h"
#include "wifi/wifi.h"


typedef union {
    uint32_t addr;
    uint8_t digit[4];
} _ip_addr;

static uint_fast8_t _wifi_status = 0;

static wifi_config_t WIFI_CFG = {
    .sta = {
        .ssid = WIFI_SSID,
        .password = WIFI_PASS,
#ifdef ENABLE_WIFI_POWERSAVE
        .listen_interval = WIFI_LISTEN_INTERVAL
#endif
    }
};
static wifi_country_t WIFI_COUNTRY = {
    .cc = "KR",
    .schan = 1,
    .nchan = 13,
    .policy = WIFI_COUNTRY_POLICY_MANUAL
};


void _wifi_handler(
    void *arg,
    esp_event_base_t event_base,
    int32_t event_id,
    void *event_data
) {
    switch (event_id) {
        case WIFI_EVENT_STA_CONNECTED:
            ESP_LOGI("WiFi-Handler", "WiFi Connected");
            _wifi_status |= 0x01;
            break;
        case IP_EVENT_STA_GOT_IP:
            _ip_addr addr;
            addr.addr = ((ip_event_got_ip_t *)event_data)->ip_info.ip.addr;
            ESP_LOGI("WiFi-Handler", "Got IP Addr %d.%d.%d.%d", addr.digit[0], addr.digit[1], addr.digit[2], addr.digit[3]);
            _wifi_status |= 0x02;
            break;
        case WIFI_EVENT_STA_DISCONNECTED:
            ESP_LOGI("WiFi-Handler", "WiFi Disconnected");
            _wifi_status &= ~0x03;
            break;
    }
}

void register_wifi_status_handler(void) {
    ESP_ERROR_CHECK(esp_event_handler_register(
        WIFI_EVENT, WIFI_EVENT_STA_CONNECTED, &_wifi_handler, NULL
    ));
    ESP_ERROR_CHECK(esp_event_handler_register(
        WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &_wifi_handler, NULL
    ));
    ESP_ERROR_CHECK(esp_event_handler_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, &_wifi_handler, NULL
    ));
}

esp_err_t init_wifi(void) {
    wifi_init_config_t wifi_init_cfg = WIFI_INIT_CONFIG_DEFAULT();

    ESP_LOGI("init_wifi", "Start init wifi...");

    ESP_ERROR_CHECK(nvs_flash_init());

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &WIFI_CFG));
    ESP_ERROR_CHECK(esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_11B|WIFI_PROTOCOL_11G|WIFI_PROTOCOL_11N));
    ESP_ERROR_CHECK(esp_wifi_set_country(&WIFI_COUNTRY));

    register_wifi_status_handler();

    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_RETURN_ON_ERROR(
        esp_wifi_connect(),
        "Wi-Fi:init_wifi", "Failed to connect wifi."
    );
    for (int i = 0; (_wifi_status & 0x03) != 0x03; i++) {
        if (i == 10) {
            ESP_LOGW("Wi-Fi:init_wifi", "Failed to connect to a Wi-Fi AP.");
            return ESP_ERR_TIMEOUT;
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }

    return ESP_OK;
}

#ifdef ENABLE_WIFI_POWERSAVE
esp_err_t enable_wifi_pm(void) {
    ESP_ERROR_CHECK(esp_wifi_set_inactive_time(WIFI_IF_STA, WIFI_BEACON_TIMEOUT));
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_MODE));
    return ESP_OK;
}
#endif

esp_err_t chech_and_reconnect_wifi(void) {
    if (!(_wifi_status & 0x01)) {
        ESP_LOGI("Wi-Fi:reconnect_wifi", "Trying to reconnect wifi...");
        ESP_RETURN_ON_ERROR(
            esp_wifi_connect(),
            "Wi-Fi:reconnect_wifi", "Failed to connect wifi."
        );
        for (int i = 0; (_wifi_status & 0x03) != 0x03; i++) {
            if (i == 10) {
                ESP_LOGW("Wi-Fi:reconnect_wifi", "WiFi connect timeout");
                return ESP_ERR_TIMEOUT;
            }
            vTaskDelay(pdMS_TO_TICKS(500));
        }
    }
    return ESP_OK;
}

esp_err_t pause_wifi(void) {
    return esp_wifi_stop();
}

esp_err_t resume_wifi(void) {
    wifi_init_config_t wifi_init_cfg = WIFI_INIT_CONFIG_DEFAULT();

    ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &WIFI_CFG));
    ESP_ERROR_CHECK(esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_11B|WIFI_PROTOCOL_11G|WIFI_PROTOCOL_11N));
    ESP_ERROR_CHECK(esp_wifi_set_country(&WIFI_COUNTRY));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_RETURN_ON_ERROR(
        esp_wifi_connect(),
        "Wi-Fi:resume_wifi", "Failed to connect wifi."
    );
    for (int i = 0; (_wifi_status & 0x03) != 0x03; i++) {
        if (i == 25) {
            ESP_LOGW("Wi-Fi:resume_wifi", "WiFi connect timeout (Status: 0x%02x)", _wifi_status);
            return ESP_ERR_TIMEOUT;
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }
    return ESP_OK;
}
