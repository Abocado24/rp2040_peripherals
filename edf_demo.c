#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "task.h"
#include "edf.h"
#include <stdio.h>

// Define task handles
TaskHandle_t task0Handle = NULL;
TaskHandle_t task1Handle = NULL;
TaskHandle_t task2Handle = NULL;

TaskHandle_t subtask0Handle = NULL;
volatile bool subtask0_spawned = false;

// Subtask functions (spawned by tasks)
void subtask0(void *pvParameters) {
    while(1) {
        TickType_t t_start = xTaskGetTickCount();

        // Simulate work
        vTaskDelay(pdMS_TO_TICKS(100));
        TickType_t t_end = xTaskGetTickCount();

        printf("Subtask 0: %d-%d\n", t_start*portTICK_PERIOD_MS, t_end*portTICK_PERIOD_MS);

        edf_complete_task(subtask0Handle);
    }
}

// Task functions
void task0(void *pvParameters) {
    bool task2_active = true;

    while (1) {
        TickType_t t_start = xTaskGetTickCount();

        // Execute subtask0 exactly once after 10000ms has passed
        if(t_start > pdMS_TO_TICKS(10000) && !subtask0_spawned) {
            subtask0_spawned = true;
            xTaskCreate(subtask0, "Subtask 0", 128, NULL, EDF_UNSELECTED_PRIORITY, &subtask0Handle);
            edf_add_task(subtask0Handle, pdMS_TO_TICKS(1000), pdMS_TO_TICKS(1000), EDF_TASK_READY);
        }

        if(task2_active) {
            edf_suspend_task(task2Handle);
        }
        else {
            edf_start_task(task2Handle);
        }
        task2_active = !task2_active;

        // Simulate work
        vTaskDelay(pdMS_TO_TICKS(500));
        TickType_t t_end = xTaskGetTickCount();

        printf("Task 0: %d-%d\n", t_start*portTICK_PERIOD_MS, t_end*portTICK_PERIOD_MS);

        edf_complete_task(task0Handle);
    }
}

void task1(void *pvParameters) {
    while (1) {
        TickType_t t_start = xTaskGetTickCount();

        // Simulate work
        vTaskDelay(pdMS_TO_TICKS(100));
        TickType_t t_end = xTaskGetTickCount();

        printf("Task 1: %d-%d\n", t_start*portTICK_PERIOD_MS, t_end*portTICK_PERIOD_MS);

        edf_complete_task(task1Handle);
    }
}

void task2(void *pvParameters) {
    while(1) {
        TickType_t t_start = xTaskGetTickCount();

        // Simulate work
        vTaskDelay(pdMS_TO_TICKS(500));
        TickType_t t_end = xTaskGetTickCount();

        printf("Task 2: %d-%d\n", t_start*portTICK_PERIOD_MS, t_end*portTICK_PERIOD_MS);

        edf_complete_task(task2Handle);
    }
}

int main() {
    stdio_init_all();

    sleep_ms(5000);

    // Create tasks with FreeRTOS but do not start them yet. They will be started by the EDF scheduler.
    xTaskCreate(task0, "Task 0", 256, NULL, EDF_UNSELECTED_PRIORITY, &task0Handle);
    xTaskCreate(task1, "Task 1", 256, NULL, EDF_UNSELECTED_PRIORITY, &task1Handle);
    xTaskCreate(task2, "Task 2", 256, NULL, EDF_UNSELECTED_PRIORITY, &task2Handle);

    // Define initial tasklist for EDF scheduler
    edf_task_t tasklist[3] = 
    {
        {.task_handle=task0Handle, .task_deadline=pdMS_TO_TICKS(3000), .task_period=pdMS_TO_TICKS(3000), .task_state=EDF_TASK_READY},
        {.task_handle=task1Handle, .task_deadline=pdMS_TO_TICKS(1000), .task_period=pdMS_TO_TICKS(0), .task_state=EDF_TASK_READY},
        {.task_handle=task2Handle, .task_deadline=pdMS_TO_TICKS(2000), .task_period=pdMS_TO_TICKS(2000), .task_state=EDF_TASK_READY},
    };

    // Start the EDF scheduler
    edf_start(tasklist, 3);

    /*  Expected result:
        Task 1 will execute first and then get deleted from the tasklist (any referrals to Task 1 from now on refer to Task 2)
        When Task 2 is suspended by Task 0, it will miss its deadline once it is resumed by Task 0. It will however recover by moving onto the current period.
        After 10000ms has passed, Task 0 will spawn a periodic subtask that is 1000ms periodic and takes 100ms to complete
    */

    while (1);
}
