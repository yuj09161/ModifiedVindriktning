#ifndef __FREERTOS_SEMAPHORE_CUSTOM_FUNCS_H_INCLUDED__
#define __FREERTOS_SEMAPHORE_CUSTOM_FUNCS_H_INCLUDED__

#ifdef __cplusplus
extern "C" {
#endif

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

BaseType_t xSemaphoreTakeN(QueueHandle_t xSemaphore, int uCount, TickType_t xTicksToWaitEach);
BaseType_t xSemaphoreGiveN(QueueHandle_t xSemaphore, int uCount);


#ifdef __cplusplus
}
#endif

#endif
