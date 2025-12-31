#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_heap_caps.h"
#include "Arduino.h"

size_t getFreeHeap() {
    return esp_get_free_heap_size();//(MALLOC_CAP_DEFAULT);
}

void checkTaskStack(TaskHandle_t handle) {
    if (handle == NULL) return;
    UBaseType_t freeStack = uxTaskGetStackHighWaterMark(handle);
    Serial.printf("Task free stack: %u bytes\n", freeStack * sizeof(StackType_t));
}