#ifndef __VINDRIKTNING_COLORS_H_INCLUDED__
#define __VINDRIKTNING_COLORS_H_INCLUDED__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef union {
    struct {
        uint8_t bright;
        uint8_t r;
        uint8_t g;
        uint8_t b;
    };
    uint32_t raw;
} led_pixel;

const led_pixel LED_COLOR_OFF    = {.bright = 0, .r =   0, .g =   0, .b =   0};
const led_pixel LED_COLOR_RED    = {.bright = 0, .r = 255, .g =   0, .b =   0};
const led_pixel LED_COLOR_GREEN  = {.bright = 0, .r =   0, .g = 255, .b =   0};
const led_pixel LED_COLOR_BLUE   = {.bright = 0, .r =   0, .g =   0, .b = 255};
const led_pixel LED_COLOR_YELLOW = {.bright = 0, .r = 255, .g = 255, .b =   0};
const led_pixel LED_COLOR_ORANGE = {.bright = 0, .r = 255, .g = 165, .b =   0};
const led_pixel LED_COLOR_WHITE  = {.bright = 0, .r = 255, .g = 255, .b = 255};

#ifdef __cplusplus
}
#endif

#endif