#include "edf.h"
#include <stdlib.h>

// EDF task array
static edf_task_t edf_tasks[EDF_MAX_TASKS];
volatile static uint8_t num_tasks = 0;
volatile int8_t current_task_idx = -1;

// Helper function that resets a task in the task array to a default state
static void reset_task(uint8_t i) {
    if(i < EDF_MAX_TASKS) {
        edf_tasks[i].task_handle = NULL;
        edf_tasks[i].task_deadline = 0;
        edf_tasks[i].task_period = 0;
        edf_tasks[i].task_state = EDF_TASK_SUSPENDED;
    }
}

// Helper function that locates the index in the task array corresponding to a task handle
static int8_t find_task(TaskHandle_t task_handle) {
    for (uint8_t i = 0; i < EDF_MAX_TASKS; i++) {
        if (edf_tasks[i].task_handle == task_handle) {
            return i;
        }
    }
    return -1;
}

// The timer's interrupt service routine (ISR) or callback function
static void edf_irq(void) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    // Give the semaphore from an ISR context to trigger scheduling
    xSemaphoreGiveFromISR(scheduler_semaphore, &xHigherPriorityTaskWoken);

    // Yield if a context switch is required
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

// Schedule active tasks according to EDF by changing task priorities
static void edf_schedule() {
    // No tasks to schedule
    if (num_tasks == 0) {
        return;
    }

    // Get the earliest deadline task that isn't paused
    TickType_t current_time = xTaskGetTickCount();
    int8_t next_task_idx = -1;
    TickType_t earliest_deadline = portMAX_DELAY;
    for (uint8_t i = 0; i < num_tasks; i++) {
        if (edf_tasks[i].task_state != EDF_TASK_SUSPENDED && 
            edf_tasks[i].task_deadline < earliest_deadline &&
            edf_tasks[i].task_deadline < (current_time + edf_tasks[i].task_period) ) {
            earliest_deadline = edf_tasks[i].task_deadline;
            next_task_idx = i;
        }
    }

    // Exit early if a switch isn't necessary
    if(next_task_idx == current_task_idx || next_task_idx == -1) {
        return;
    }

    // Adjust the states and priorities of tasks
    vTaskPrioritySet(edf_tasks[current_task_idx].task_handle, EDF_UNSELECTED_PRIORITY);
    edf_tasks[current_task_idx].task_state = EDF_TASK_READY;

    vTaskPrioritySet(edf_tasks[next_task_idx].task_handle, EDF_SELECTED_PRIORITY);
    edf_tasks[next_task_idx].task_state = EDF_TASK_RUNNING;

    // Update current task being run
    current_task_idx = next_task_idx;
}

// Dedicated task for scheduling
static void edf_scheduler_task(void *pvParameters) {
    while (1) {
        // Wait indefinitely for the semaphore to be given
        if (xSemaphoreTake(scheduler_semaphore, portMAX_DELAY) == pdTRUE) {
            // Run the EDF scheduling logic
            edf_schedule();
        }
    }
}

// Initialize the EDF scheduler with no tasks added
void edf_init(void) {
    num_tasks = 0;
    // Initialize EDF task array (all tasks should be a default, unset state)
    for(uint8_t i = 0; i < EDF_MAX_TASKS; i++) {
        reset_task(i);
    }
}

// Add a new task to the scheduler
// Returns true on success, false otherwise
bool edf_add_task(TaskHandle_t task_handle, TickType_t task_deadline, TickType_t task_period, edf_task_state_t task_state) {
    // Indicate failure to add new task (max tasks reached)
    if(num_tasks >= EDF_MAX_TASKS) {
        return false;
    }
    // Indicate failure to add new task (task already exists in scheduler)
    else if(find_task(task_handle) >= 0) {
        return false;
    }
    // Indicate failure to add new task (illogical task deadline or period)
    else if(task_deadline == portMAX_DELAY || task_period == portMAX_DELAY) {
        return false;
    }
    
    TickType_t current_time = xTaskGetTickCount();

    // Add task to the end of the task array
    edf_tasks[num_tasks].task_handle = task_handle;
    edf_tasks[num_tasks].task_deadline = current_time + task_deadline;
    edf_tasks[num_tasks].task_period = task_period;
    edf_tasks[num_tasks].task_state = task_state;
    num_tasks++;

    return true;
}

// Start a task in the scheduler
// Returns true on success, false otherwise
bool edf_start_task(TaskHandle_t task_handle) {
    // Locate task location in task array
    int8_t task_idx = find_task(task_handle);

    // Indicate failure to locate task
    if(task_idx < 0) {
        return false;
    }

    // Directly suspend the task in FreeRTOS if applicable
    if(edf_tasks[task_idx].task_state == EDF_TASK_SUSPENDED) {
        vTaskResume(task_handle);
    }

    // Update task state within the EDF scheduler
    edf_tasks[task_idx].task_state = EDF_TASK_READY;

    return true;
}

// Pause a task in the scheduler
// Returns true on success, false otherwise
bool edf_pause_task(TaskHandle_t task_handle) {
    // Locate task location in task array
    int8_t task_idx = find_task(task_handle);

    // Indicate failure to locate task
    if(task_idx < 0) {
        return false;
    }

    // Update task state within the EDF scheduler
    edf_tasks[task_idx].task_state = EDF_TASK_SUSPENDED;

    // Directly suspend the task in FreeRTOS
    vTaskSuspend(task_handle);

    return true;
}

// Remove a task from the scheduler
bool edf_remove_task(TaskHandle_t task_handle) {
    // Locate task location in task array
    int8_t task_idx = find_task(task_handle);

    // Indicate failure to locate task
    if(task_idx < 0) {
        return false;
    }

    // Shift tasks one position to the left starting from the removed task's position
    for (uint8_t i = task_idx; i < num_tasks - 1; i++) {
        edf_tasks[i] = edf_tasks[i + 1];
    }

    // Remove the task at the end
    reset_task(num_tasks - 1);
    num_tasks--;

    return true;
}

// Mark a task as completed in the scheduler
bool edf_task_completed(TaskHandle_t task_handle) {
    // Locate task location in task array
    int8_t task_idx = find_task(task_handle);

    // Indicate failure to locate task
    if(task_idx < 0) {
        return false;
    }

    // If the task is non-periodic, remove it from the scheduler
    if(edf_tasks[task_idx].task_period == 0) {
        if(!edf_remove_task(task_handle)) {
            return false;
        }
    }
    else {
        // Otherwise update the task's deadline for periodic tasks
        TickType_t current_time = xTaskGetTickCount();
        edf_tasks[task_idx].task_deadline += edf_tasks[task_idx].task_period;
    }

    // Signal the scheduler to run again, as the task set has changed
    xSemaphoreGive(scheduler_semaphore);

    return true;
}

// Start the EDF scheduler by creating the scheduler task
void edf_start(uint32_t timer_period_us) {
    scheduler_semaphore = xSemaphoreCreateBinary();
    if(scheduler_semaphore != NULL) {
        xTaskCreate(SchedulerTask, "EDFScheduler", EDF_SCHEDULER_STACK_SIZE, NULL, EDF_SCHEDULER_PRIORITY, NULL);
    }

    alarm_pool_add_repeating_timer_us(get_default_alarm_pool(), timer_period_us, (alarm_callback_t)edf_irq, NULL, &scheduler_timer)
}