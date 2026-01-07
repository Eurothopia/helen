#pragma once
#include "Arduino.h"
#include "Ticker.h"
#include "freertos/task.h"

#include <definitions.h>
//#include <global.h>

//extern bool boosting;


//compatibility type, locks onto idle task
inline float get_cpu_usage(int core) {
    static uint32_t last_idle_time[2] = {0, 0};
    static uint32_t last_total_time[2] = {0, 0};
    
    TaskStatus_t xTaskStatus;
    // Get the handle for the Idle task of the specific core
    // Core 0 Idle Task is usually named "IDLE0", Core 1 is "IDLE1"
    TaskHandle_t idleHandle = xTaskGetIdleTaskHandleForCore(core);
    
    vTaskGetInfo(idleHandle, &xTaskStatus, pdTRUE, eInvalid);
    
    uint32_t now = esp_timer_get_time(); // Time in microseconds
    uint32_t idle_time = xTaskStatus.ulRunTimeCounter; // This requires configGENERATE_RUN_TIME_STATS
    
    // If your Arduino setup has RUN_TIME_STATS disabled (common), 
    // we use a different approach. See "The Manual Way" below if this returns 0.
    
    float usage = 100.0 * (1.0 - (float)(idle_time - last_idle_time[core]) / (now - last_total_time[core]));
    
    last_idle_time[core] = idle_time;
    last_total_time[core] = now;
    
    return usage;
}

//non invasive
inline void print_cpu_stats() {
    TaskStatus_t *pxTaskStatusArray;
    volatile UBaseType_t uxArraySize, x;
    uint32_t ulTotalRunTime;

    // 1. Get number of tasks
    uxArraySize = uxTaskGetNumberOfTasks();
    pxTaskStatusArray = (TaskStatus_t *)pvPortMalloc(uxArraySize * sizeof(TaskStatus_t));

    if (pxTaskStatusArray != NULL) {
        // 2. Generate snapshots of all tasks
        uxArraySize = uxTaskGetSystemState(pxTaskStatusArray, uxArraySize, &ulTotalRunTime);

        // 3. To get percentage, we need the "Total Run Time" (ulTotalRunTime)
        // Divide by 100 to get a scale
        ulTotalRunTime /= 100;

        if (ulTotalRunTime > 0) {
            for (x = 0; x < uxArraySize; x++) {
                int stats = pxTaskStatusArray[x].ulRunTimeCounter / ulTotalRunTime;
                
                // If the task is "IDLE", 100 - stats = Busy %
                Serial.printf("Task: %-15s | Core: %d | Usage: %d%%\n", 
                              pxTaskStatusArray[x].pcTaskName, 
                              pxTaskStatusArray[x].xCoreID,
                              stats);
            }
        }
        vPortFree(pxTaskStatusArray);
    }
}