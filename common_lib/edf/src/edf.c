#include "edf.h"
#include <stdlib.h>

// EDF task array
static edf_task_t edf_tasks[EDF_MAX_TASKS];
volatile static uint8_t num_tasks = 0;

volatile bool scheduler_started;

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
static int8_t find_task_idx(TaskHandle_t task_handle) {
    for (uint8_t i = 0; i < EDF_MAX_TASKS; i++) {
        if (edf_tasks[i].task_handle == task_handle) {
            return i;
        }
    }
    return -1;
}

// Print out deadlines for all tasks (debugging)
static void print_deadlines() {
    TickType_t current_time = xTaskGetTickCount();

    for(uint8_t i=0; i<num_tasks; i++) {
        if(edf_tasks[i].task_state != EDF_TASK_SUSPENDED) {
            printf("%d\t", ((edf_tasks[i].task_deadline) - current_time)*portTICK_PERIOD_MS);
        }
        else {
            printf("N/A\t");
        }
    }
    printf("\n");
}

// Helper function that determines the next task to work on for EDF
static int8_t get_next_task_idx() {
    // Defaults to this value if no task should currently be worked on
    int8_t next_task_idx = -1;

    TickType_t current_time = xTaskGetTickCount();
    TickType_t earliest_deadline = portMAX_DELAY;

    for(uint8_t i=0; i<num_tasks; i++) {
        if (edf_tasks[i].task_state != EDF_TASK_SUSPENDED   &&      // Is a non-paused task
            edf_tasks[i].task_deadline < earliest_deadline          // Has the new earliest deadline 
            ) {
            earliest_deadline = edf_tasks[i].task_deadline;
            next_task_idx = i;
        }
    }

    return next_task_idx;
}

// Helper function that switches from the current task to an indicated next task using FreeRTOS
static void switch_task(int8_t current_task_idx, int8_t next_task_idx) {
    // Exit early if a switch isn't necessary
    if(next_task_idx == current_task_idx) {
        return;
    }

    // Lower priority of current task if it isn't no task
    if(current_task_idx != -1) {
        vTaskPrioritySet(edf_tasks[current_task_idx].task_handle, EDF_UNSELECTED_PRIORITY);
        edf_tasks[current_task_idx].task_state = EDF_TASK_READY;
    }
    // Raise priority of next task if it isn't no task
    if(next_task_idx != -1) {
        vTaskPrioritySet(edf_tasks[next_task_idx].task_handle, EDF_SELECTED_PRIORITY);
        edf_tasks[next_task_idx].task_state = EDF_TASK_RUNNING;
    }
}

// Schedule active tasks according to EDF by changing task priorities
void edf_schedule() {
    print_deadlines();

    // No tasks to schedule
    if (num_tasks == 0) {
        return;
    }

    // Start at no task selected, need to keep track of current state between schedules
    static int8_t current_task_idx = -1;

    int8_t next_task_idx = get_next_task_idx();
    switch_task(current_task_idx, next_task_idx);

    // Update current task being run
    current_task_idx = next_task_idx;
}

// Initialize the EDF scheduler with no tasks added
void edf_init(void) {
    // Initialize global objects
    num_tasks = 0;
    scheduler_started = false;

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
    else if(find_task_idx(task_handle) >= 0) {
        return false;
    }
    // Indicate failure to add new task (illogical tick input)
    else if(task_deadline == portMAX_DELAY || task_period == portMAX_DELAY) {
        return false;
    }
    // Indicate failure to add new task (shouldn't create one at EDF_TASK_RUNNING state)
    else if(task_state == EDF_TASK_RUNNING) {
        return false;
    }
    
    // Add task to the end of the task array
    edf_tasks[num_tasks].task_handle = task_handle;
    edf_tasks[num_tasks].task_deadline = xTaskGetTickCount() + task_deadline;
    edf_tasks[num_tasks].task_period = task_period;
    edf_tasks[num_tasks].task_state = task_state;
    num_tasks++;

    // Reschedule as the tasklist has changed
    edf_schedule();

    return true;
}

// Start a task in the scheduler
// Returns true on success, false otherwise
bool edf_start_task(TaskHandle_t task_handle) {
    // Locate task location in task array
    int8_t task_idx = find_task_idx(task_handle);

    // Indicate failure to locate task
    if(task_idx < 0) {
        return false;
    }

    // Directly resume the task in FreeRTOS if applicable
    if(scheduler_started && edf_tasks[task_idx].task_state == EDF_TASK_SUSPENDED) {
        vTaskResume(task_handle);
    }

    // Update task state within the EDF scheduler
    edf_tasks[task_idx].task_state = EDF_TASK_READY;

    // Reschedule as the tasklist has changed
    edf_schedule();

    return true;
}

// Pause a task in the scheduler
// Returns true on success, false otherwise
bool edf_pause_task(TaskHandle_t task_handle) {
    // Locate task location in task array
    int8_t task_idx = find_task_idx(task_handle);

    // Indicate failure to locate task
    if(task_idx < 0) {
        return false;
    }

    // Update task state within the EDF scheduler
    edf_tasks[task_idx].task_state = EDF_TASK_SUSPENDED;

    // Directly suspend the task in FreeRTOS if applicable
    if(scheduler_started) {
        vTaskSuspend(task_handle);
    }

    // Reschedule as the tasklist has changed
    edf_schedule();

    return true;
}

// Remove a task from the scheduler
bool edf_remove_task(TaskHandle_t task_handle) {
    // Locate task location in task array
    int8_t task_idx = find_task_idx(task_handle);

    // Indicate failure to locate task
    if(task_idx < 0) {
        return false;
    }

    // Shift tasks one position to the left starting from the removed task's position
    for (uint8_t i = task_idx; i < num_tasks - 1; i++) {
        edf_tasks[i] = edf_tasks[i + 1];
    }

    // Remove task from array
    reset_task(num_tasks - 1);
    num_tasks--;

    // Reschedule as the tasklist has changed
    edf_schedule();

    return true;
}

// Mark a task as completed in the scheduler
// Assumes that this this function is called in the current task
bool edf_task_completed(TaskHandle_t task_handle) {    
    // Locate task location in task array
    int8_t task_idx = find_task_idx(task_handle);

    // Indicate failure to locate task
    if(task_idx < 0) {
        return false;
    }

    TickType_t delay_time = 0;

    // Add periodic task back into the tasklist
    if(edf_tasks[task_idx].task_period > 0) {
        delay_time = edf_tasks[task_idx].task_deadline - xTaskGetTickCount();

        // Update deadline for next period
        edf_tasks[task_idx].task_deadline += edf_tasks[task_idx].task_period;
    }
    else {
        // Remove non-periodic task from scheduler
        if(!edf_remove_task(task_handle)) {
            return false;
        }

        // Delete task via FreeRTOS
        vTaskDelete(task_handle);
    }

    // Reschedule as the tasklist has changed
    edf_schedule();

    vTaskDelay(delay_time);

    return true;
}

// Start the EDF scheduler by creating the scheduler task
void edf_start() {
    scheduler_started = true;

    // Start the scheduler
    vTaskStartScheduler();
}