#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "task.h"
#include "edf.h"
#include <stdio.h>

// Define task handles
TaskHandle_t task0Handle = NULL;
TaskHandle_t task1Handle = NULL;
TaskHandle_t task2Handle = NULL;

// Task functions
void task0(void *pvParameters) {
    while (1) {
        TickType_t t_start = xTaskGetTickCount();

        TickType_t t_end = t_start;
        while((t_end - t_start) <  pdMS_TO_TICKS(500)) {
            t_end = xTaskGetTickCount();
        }

        printf("Task 0: %d-%d\n", t_start*portTICK_PERIOD_MS, t_end*portTICK_PERIOD_MS);

        edf_task_completed(task0Handle);
    }
}

void task1(void *pvParameters) {
    while (1) {
        TickType_t t_start = xTaskGetTickCount();

        TickType_t t_end = t_start;
        while((t_end - t_start) <  pdMS_TO_TICKS(500)) {
            t_end = xTaskGetTickCount();
        }

        printf("Task 1: %d-%d\n", t_start*portTICK_PERIOD_MS, t_end*portTICK_PERIOD_MS);

        edf_task_completed(task1Handle);
    }
}

void task2(void *pvParameters) {
    while(1) {
        TickType_t t_start = xTaskGetTickCount();

        TickType_t t_end = t_start;
        while((t_end - t_start) <  pdMS_TO_TICKS(500)) {
            t_end = xTaskGetTickCount();
        }

        printf("Task 2: %d-%d\n", t_start*portTICK_PERIOD_MS, t_end*portTICK_PERIOD_MS);

        edf_task_completed(task2Handle);
    }
}

int main() {
    stdio_init_all();

    sleep_ms(5000);

    edf_init(); // Initialize the EDF scheduler

    // Create tasks with FreeRTOS but do not start them yet. They will be started by the EDF scheduler.
    xTaskCreate(task0, "Task 0", 256, NULL, EDF_UNSELECTED_PRIORITY, &task0Handle);
    xTaskCreate(task1, "Task 1", 256, NULL, EDF_UNSELECTED_PRIORITY, &task1Handle);
    xTaskCreate(task2, "Task 2", 256, NULL, EDF_UNSELECTED_PRIORITY, &task2Handle);

    // Add tasks to the EDF scheduler with their deadlines and periods
    edf_add_task(task0Handle, pdMS_TO_TICKS(5000), pdMS_TO_TICKS(5000), EDF_TASK_READY);
    edf_add_task(task1Handle, pdMS_TO_TICKS(2000), pdMS_TO_TICKS(2000), EDF_TASK_READY);
    edf_add_task(task2Handle, pdMS_TO_TICKS(3000), pdMS_TO_TICKS(3000), EDF_TASK_READY);

    // Start the EDF scheduler
    edf_start();

    while (1);
}
