#ifndef __ST_COMMON_WIFI_H_INCLUDED__
#define __ST_COMMON_WIFI_H_INCLUDED__

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"

void register_wifi_status_handler(void);
esp_err_t init_wifi(void);
esp_err_t chech_and_reconnect_wifi(void);
esp_err_t pause_wifi(void);
esp_err_t resume_wifi(void);

#ifdef __cplusplus
}
#endif

#endif