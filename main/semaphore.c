#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "semaphore.h"


BaseType_t xSemaphoreTakeN(QueueHandle_t xSemaphore, int uCount, TickType_t xTicksToWaitEach) {
    ESP_LOGD("Main:xSemaphoreTakeN", "Semaphore Count (Start): %d", uxSemaphoreGetCount(xSemaphore));
    for (int i = 0; i < uCount; i++) {
        ESP_LOGD("Main:xSemaphoreTakeN", "Semaphore Count (Before call): %d", uxSemaphoreGetCount(xSemaphore));
        if (!xSemaphoreTake(xSemaphore, xTicksToWaitEach)) {
            ESP_LOGD("Main:xSemaphoreTakeN", "Semaphore Count (After call): %d", uxSemaphoreGetCount(xSemaphore));
            return pdFALSE;
        }
    }
    ESP_LOGD("Main:xSemaphoreTakeN", "Successfully taken %d times.", uCount);
    return pdTRUE;
}

BaseType_t xSemaphoreGiveN(QueueHandle_t xSemaphore, int uCount) {
    ESP_LOGD("Main:xSemaphoreGiveN", "Semaphore Count (Start): %d", uxSemaphoreGetCount(xSemaphore));
    for (int i = 0; i < uCount; i++) {
        ESP_LOGD("Main:xSemaphoreGiveN", "Semaphore Count (Before call): %d", uxSemaphoreGetCount(xSemaphore));
        if (!xSemaphoreGive(xSemaphore)) {
            ESP_LOGD("Main:xSemaphoreGiveN", "Semaphore Count (After call): %d", uxSemaphoreGetCount(xSemaphore));
            return pdFALSE;
        }
    }
    ESP_LOGD("Main:xSemaphoreGiveN", "Successfully given %d times.", uCount);
    return pdTRUE;
}
