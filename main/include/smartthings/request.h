#ifndef __ST_COMMON_REQUEST_H_INCLUDED__
#define __ST_COMMON_REQUEST_H_INCLUDED__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int switchLevel;
    int fanSpeed;
} STStatus;


typedef struct {
    int tempHigh;
    int tempLow;
    int humiHigh;
    int humiLow;
    int fineDustVeryBad;
    int fineDustBad;
    int fineDustWarning;
    int fineDustNormal;
    int illuminanceHigh;
    int illuminanceLow;
    struct {
        float temperature;
        int humidity;
        int tvoc;
        float temperature2;
        float pressure;
    } offsets;
} DeviceConfig;

esp_err_t get_device_status(STStatus *result);
esp_err_t get_device_config(DeviceConfig *result);
esp_err_t set_device_status(
    int fine_dust,
    float temperature,
    int humidity,
    int tvoc,
    float temperature2,
    float pressure
);

#ifdef __cplusplus
}
#endif

#endif
