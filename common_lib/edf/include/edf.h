#ifndef EDF_H
#define EDF_H

#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <queue.h>

#include "pico/stdlib.h"
#include "hardware/timer.h"
#include "hardware/irq.h"
#include "pico/time.h"

#define EDF_MAX_TASKS                               10
#define EDF_SCHEDULER_ACTION_QUEUE_LENGTH           10
#define EDF_SCHEDULER_PRIORITY                      (configMAX_PRIORITIES - 1)
#define EDF_SELECTED_PRIORITY                       (configMAX_PRIORITIES - 2)
#define EDF_UNSELECTED_PRIORITY                     (configMAX_PRIORITIES - 3)
#define EDF_SCHEDULER_STACK_SIZE                    2048

typedef enum {
    EDF_SCHEDULER_ACTION_ADD_TASK = 0,
    EDF_SCHEDULER_ACTION_START_TASK = 1,
    EDF_SCHEDULER_ACTION_SUSPEND_TASK = 2,
    EDF_SCHEDULER_ACTION_DELETE_TASK = 3,
    EDF_SCHEDULER_ACTION_COMPLETE_TASK = 4,
} edf_scheduler_action_type_t;

typedef struct {
    // Core info
    edf_scheduler_action_type_t action_type;
    TaskHandle_t task_handle;
    // Additional info (only really needed for add_task)
    TickType_t task_deadline;
    TickType_t task_period;
    bool is_suspended;
} edf_scheduler_action_t;

typedef enum {
    EDF_TASK_ERROR = -1,
    EDF_TASK_READY = 0,
    EDF_TASK_RUNNING = 1,
    EDF_TASK_SUSPENDED = 2,
} edf_task_state_t;

typedef struct {
    TaskHandle_t task_handle;
    TickType_t task_deadline;
    TickType_t task_period;
    edf_task_state_t task_state;
} edf_task_t;

bool edf_start(edf_task_t tasklist[], uint8_t tasklist_length);
bool edf_add_task(TaskHandle_t task_handle, TickType_t task_deadline, TickType_t task_period, bool is_suspended);
bool edf_start_task(TaskHandle_t task_handle);
bool edf_suspend_task(TaskHandle_t task_handle);
bool edf_delete_task(TaskHandle_t task_handle);
bool edf_complete_task(TaskHandle_t task_handle);

#endif // EDF_H