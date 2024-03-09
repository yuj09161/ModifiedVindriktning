#include <stdio.h>
#include <stdint.h>
#include <limits.h>
#include <float.h>

#include "esp_log.h"
#include "esp_check.h"
#include "cJSON.h"

#include "config.h"
#include "smartthings/rest.h"
#include "smartthings/request.h"

#if defined(CONFIG_HEAP_TRACING) && CONFIG_LOG_DEFAULT_LEVEL >= 4  // When default log level is verbose than DEBUG
#include "esp_heap_trace.h"
#endif


#define DEVICE_STATUS_TO_STR(component, capability, attribute, value, unit)\
    "{\"component\":\""component"\",\"capability\":\""capability"\",\"attribute\":\""attribute"\",\"value\":"value",\"unit\":\""unit"\"}"

typedef struct {
    char *component;
    char *capability;
    char *attribute;
    char *value;
    char *unit;
} st_item;

esp_err_t get_device_status(STStatus *result) {
#if CONFIG_LOG_DEFAULT_LEVEL >= 4  // When default log level is verbose than DEBUG
        ESP_LOGD("ST_Request:get_device_status", "*****Free Mem (Start): %u*****", heap_caps_get_free_size(MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT));
#ifdef CONFIG_HEAP_TRACING
        ESP_ERROR_CHECK(heap_trace_start(HEAP_TRACE_LEAKS));
#endif
#endif
    cJSON *response_json = NULL;

    esp_err_t ret = ESP_OK;
    ESP_LOGI("ST-REQUEST get_device_status", "Sending response...");
    ESP_GOTO_ON_ERROR(
        get(ST_ACCESS_TOKEN, DEVICE_MAIN_COMPONENT_STATUS_URL(ST_DEVICE_ID), &response_json),
        CLEANUP, "ST-REQUEST", "Get status failed."
    );
    ESP_LOGI("ST-REQUEST get_device_status", "Successfully sent.");

    if (esp_log_level_get("ST-REQUEST get_device_status") >= ESP_LOG_DEBUG) {
        char *json_str = cJSON_Print(response_json);
        puts(json_str);
        free(json_str);
    }
    ESP_LOGD("ST-REQUEST get_device_status", "Result printed.");

    cJSON *switch_obj = cJSON_GetObjectItemCaseSensitive(response_json, "switchLevel");
    cJSON *switch_level_obj = cJSON_GetObjectItemCaseSensitive(switch_obj, "level");
    result->switchLevel = cJSON_GetObjectItemCaseSensitive(switch_level_obj, "value")->valueint;

    cJSON *fanSpeed_obj = cJSON_GetObjectItemCaseSensitive(response_json, "fanSpeed");
    cJSON *fanSpeed_fanSpeed_obj = cJSON_GetObjectItemCaseSensitive(fanSpeed_obj, "fanSpeed");
    result->fanSpeed = cJSON_GetObjectItemCaseSensitive(fanSpeed_fanSpeed_obj, "value")->valueint;

CLEANUP:
    if (response_json != NULL)
        cJSON_Delete(response_json);
#if CONFIG_LOG_DEFAULT_LEVEL >= 4  // When default log level is verbose than DEBUG
        ESP_LOGD("ST_Request:get_device_status", "*****Free Mem ( End ): %u*****", heap_caps_get_free_size(MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT));
#ifdef CONFIG_HEAP_TRACING
        ESP_ERROR_CHECK(heap_trace_stop());
        heap_trace_dump();
#endif
#endif
    return ret;
}

esp_err_t get_device_config(DeviceConfig *result) {
#if CONFIG_LOG_DEFAULT_LEVEL >= 4  // When default log level is verbose than DEBUG
#ifdef CONFIG_HEAP_TRACING
        ESP_ERROR_CHECK(heap_trace_start(HEAP_TRACE_LEAKS));
#endif
        ESP_LOGD("ST_Request:get_device_config", "*****Free Mem (Start): %u*****", heap_caps_get_free_size(MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT));
#endif
    cJSON *response_json = NULL;

    esp_err_t ret = ESP_OK;
    ESP_LOGI("ST-REQUEST get_device_config", "Sending response...");
    ESP_GOTO_ON_ERROR(
        get(ST_ACCESS_TOKEN, DEVICE_PREFERENCES_URL(ST_DEVICE_ID), &response_json),
        CLEANUP, "ST-REQUEST", "Get config failed."
    );
    ESP_LOGI("ST-REQUEST get_device_config", "Successfully sent.");

    if (esp_log_level_get("ST-REQUEST get_device_config") >= ESP_LOG_DEBUG) {
        char *json_str = cJSON_Print(response_json);
        puts(json_str);
        free(json_str);
    }
    ESP_LOGD("ST-REQUEST get_device_config", "Result printed.");

    cJSON *values = cJSON_GetObjectItemCaseSensitive(response_json, "values");
    result->tempHigh        = cJSON_GetObjectItemCaseSensitive(
        cJSON_GetObjectItemCaseSensitive(values, "tempHigh"), "value"
    )->valueint;
    result->tempLow         = cJSON_GetObjectItemCaseSensitive(
        cJSON_GetObjectItemCaseSensitive(values, "tempLow"), "value"
    )->valueint;
    result->humiHigh        = cJSON_GetObjectItemCaseSensitive(
        cJSON_GetObjectItemCaseSensitive(values, "humiHigh"), "value"
    )->valueint;
    result->humiLow         = cJSON_GetObjectItemCaseSensitive(
        cJSON_GetObjectItemCaseSensitive(values, "humiLow"), "value"
    )->valueint;
    result->fineDustVeryBad = cJSON_GetObjectItemCaseSensitive(
        cJSON_GetObjectItemCaseSensitive(values, "fineDustVeryBad"), "value"
    )->valueint;
    result->fineDustBad     = cJSON_GetObjectItemCaseSensitive(
        cJSON_GetObjectItemCaseSensitive(values, "fineDustBad"), "value"
    )->valueint;
    result->fineDustWarning = cJSON_GetObjectItemCaseSensitive(
        cJSON_GetObjectItemCaseSensitive(values, "fineDustWarning"), "value"
    )->valueint;
    result->fineDustNormal  = cJSON_GetObjectItemCaseSensitive(
        cJSON_GetObjectItemCaseSensitive(values, "fineDustNormal"), "value"
    )->valueint;
    result->illuminanceHigh = cJSON_GetObjectItemCaseSensitive(
        cJSON_GetObjectItemCaseSensitive(values, "illuminanceHigh"), "value"
    )->valueint;
    result->illuminanceLow  = cJSON_GetObjectItemCaseSensitive(
        cJSON_GetObjectItemCaseSensitive(values, "illuminanceLow"), "value"
    )->valueint;

    result->offsets.temperature  = cJSON_GetObjectItemCaseSensitive(
        cJSON_GetObjectItemCaseSensitive(values, "temperatureOffset"), "value"
    )->valuedouble;
    result->offsets.humidity     = cJSON_GetObjectItemCaseSensitive(
        cJSON_GetObjectItemCaseSensitive(values, "humidityOffset"), "value"
    )->valueint;
    result->offsets.tvoc         = cJSON_GetObjectItemCaseSensitive(
        cJSON_GetObjectItemCaseSensitive(values, "tvocOffset"), "value"
    )->valueint;
    result->offsets.temperature2 = cJSON_GetObjectItemCaseSensitive(
        cJSON_GetObjectItemCaseSensitive(values, "temperature2Offset"), "value"
    )->valuedouble;
    result->offsets.pressure     = cJSON_GetObjectItemCaseSensitive(
        cJSON_GetObjectItemCaseSensitive(values, "pressureOffset"), "value"
    )->valuedouble;

CLEANUP:
    if (response_json != NULL)
        cJSON_Delete(response_json);
#if CONFIG_LOG_DEFAULT_LEVEL >= 4  // When default log level is verbose than DEBUG
        ESP_LOGD("ST_Request:get_device_config", "*****Free Mem ( End ): %u*****", heap_caps_get_free_size(MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT));
#ifdef CONFIG_HEAP_TRACING
        ESP_ERROR_CHECK(heap_trace_stop());
        heap_trace_dump();
#endif
#endif
    return ret;
}

esp_err_t _create_request_body(st_item *items, int item_cnt, char *buf, int buf_size) {
    int buf_pos = 18, item_str_len;
    strcpy(buf, "{\"deviceEvents\": [");
    for (int i = 0; i < item_cnt; i++) {
        if (
            items[i].value[0] == '\0'
            || items[i].value == NULL
            || items[i].component == NULL
            || items[i].capability == NULL
            || items[i].attribute == NULL
            || items[i].unit == NULL
        ) {
            ESP_LOGI("ST-REQUEST create_request_body", "Invalid value detected @ item[%d]. Skipping...", i);
            continue;
        }

        ESP_LOGD("ST-REQUEST create_request_body", "Formatting item[%d]...", i);
        item_str_len = snprintf(
            buf + buf_pos, buf_size - buf_pos - 3,
            "{\"component\":\"%s\",\"capability\":\"%s\",\"attribute\":\"%s\",\"value\":%s,\"unit\":\"%s\"},",
            items[i].component, items[i].capability, items[i].attribute, items[i].value, items[i].unit
        );
        if (buf_pos + item_str_len > buf_size - 2) {
            ESP_LOGW(
                "ST-REQUEST create_request_body",
                "Buffer is to small (buf_size: %d | buf_pos: %d | item_no: %d | item_str_len: %d)",
                buf_size, buf_pos, i, item_str_len
            );
            return ESP_ERR_NO_MEM;
        }
        buf_pos += item_str_len;
    }
    strcpy(buf + buf_pos - 1, "]}");
    return ESP_OK;
}

esp_err_t set_device_status(
    int fine_dust,
    float temperature,
    int humidity,
    int tvoc,
    float temperature2,
    float pressure
) {
#if CONFIG_LOG_DEFAULT_LEVEL >= 4  // When default log level is verbose than DEBUG
#ifdef CONFIG_HEAP_TRACING
        ESP_ERROR_CHECK(heap_trace_start(HEAP_TRACE_LEAKS));
#endif
        ESP_LOGD("ST_Request:set_device_status", "*****Free Mem (Start): %u*****", heap_caps_get_free_size(MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT));
#endif
    char *body_buf;
    char fine_dust_str[12], temperature_str[12], humidity_str[12], tvoc_str[12], temperature2_str[12], pressure_str[12];

    // For indicate invalid value
    if (fine_dust == INT_MIN) {
        fine_dust_str[0] = '\0';
    } else {
        sprintf(fine_dust_str, "%d", fine_dust);
    }
    if (temperature == FLT_MIN) {
        temperature_str[0] = '\0';
    } else {
        sprintf(temperature_str, "%.1f", temperature);
    }
    if (humidity == INT_MIN) {
        humidity_str[0] = '\0';
    } else {
        sprintf(humidity_str, "%d", humidity);
    }
    if (tvoc == INT_MIN) {
        tvoc_str[0] = '\0';
    } else {
        sprintf(tvoc_str, "%d", tvoc);
    }
    if (temperature2 == FLT_MIN) {
        temperature2_str[0] = '\0';
    } else {
        sprintf(temperature2_str, "%.1f", temperature2);
    }
    if (pressure == FLT_MIN) {
        pressure_str[0] = '\0';
    } else {
        sprintf(pressure_str, "%.1f", pressure);
    }
    st_item items[6] = {
        {
            .component  = "airQuality",
            .capability = "fineDustSensor",
            .attribute  = "fineDustLevel",
            .value      = fine_dust_str,
            .unit       = "μg/m^3"
        },
        {
            .component  = "airQuality",
            .capability = "temperatureMeasurement",
            .attribute  = "temperature",
            .value      = temperature_str,
            .unit       = "C"
        },
        {
            .component  = "airQuality",
            .capability = "relativeHumidityMeasurement",
            .attribute  = "humidity",
            .value      = humidity_str,
            .unit       = "%"
        },
        {
            .component  = "airQuality",
            .capability = "tvocMeasurement",
            .attribute  = "tvocLevel",
            .value      = tvoc_str,
            .unit       = "ppb"
        },
        {
            .component  = "airPressure",
            .capability = "temperatureMeasurement",
            .attribute  = "temperature",
            .value      = temperature2_str,
            .unit       = "C"
        },
        {
            .component  = "airPressure",
            .capability = "atmosphericPressureMeasurement",
            .attribute  = "atmosphericPressure",
            .value      = pressure_str,
            .unit       = "kPa"
        }
    };

    esp_err_t ret = ESP_OK;
    body_buf = malloc(sizeof(char) * SET_DEVICE_STATUS_BUF_SIZE);
    ESP_GOTO_ON_FALSE(
        body_buf != NULL, ESP_ERR_NO_MEM, CLEANUP,
        "ST-REQUEST", "Insufficient memory for body_buf."
    );

    ESP_GOTO_ON_ERROR(
        _create_request_body(items, 6, body_buf, SET_DEVICE_STATUS_BUF_SIZE), CLEANUP,
        "ST-REQUEST", "Error occured while create request body."
    );

    ESP_LOGD("ST-REQUEST set_device_status", "Successfully formatted body. body:");
    if (esp_log_level_get("ST-REQUEST set_device_status") >= ESP_LOG_DEBUG)
        puts(body_buf);

    ESP_GOTO_ON_ERROR(
        post(ST_ACCESS_TOKEN, VIRTUALDEVICE_EVENT_URL(ST_DEVICE_ID), body_buf, NULL), CLEANUP,
        "ST-REQUEST", "Error occured while sending events."
    );

CLEANUP:
    free(body_buf);
#if CONFIG_LOG_DEFAULT_LEVEL >= 4  // When default log level is verbose than DEBUG
        ESP_LOGD("ST_Request:set_device_status", "*****Free Mem ( End ): %u*****", heap_caps_get_free_size(MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT));
#ifdef CONFIG_HEAP_TRACING
        ESP_ERROR_CHECK(heap_trace_stop());
        heap_trace_dump();
#endif
#endif
    return ret;
}
