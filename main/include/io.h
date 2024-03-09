#ifndef __VINDRIKTNING_IO_H_INCLUDED__
#define __VINDRIKTNING_IO_H_INCLUDED__

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"
#include "colors.h"

#include "pwm_led.h"
#include "ws2812.h"

typedef struct {
    int fine_dust;
    float temperature;
    int humidity;
    float temperature2;
    float pressure;
    int tvoc;
} sensor_values;

void init_gpio(void);
void init_modules(PWMLed **statusLED, WS2812Strip **strip);
esp_err_t init_sensors(void);

esp_err_t get_sensor_values(sensor_values *result);
esp_err_t set_strip_pixels(led_pixel *pixels);

esp_err_t fan_off(void);
esp_err_t fan_on(void);

#ifdef __cplusplus
}
#endif

#endif