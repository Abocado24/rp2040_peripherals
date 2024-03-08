#include "edf.h"
#include <stdlib.h>

// EDF task array
static edf_task_t edf_tasks[EDF_MAX_TASKS];
volatile static uint8_t num_tasks = 0;

// Scheduler task and action queue
TaskHandle_t scheduler_task_handle;
QueueHandle_t scheduler_action_queue;

// Flags
volatile int8_t current_task_idx = -1;
volatile bool scheduler_started;

// Helper function that resets a task in the task array to a default state
static void reset_task(uint8_t i) {
    if(i < EDF_MAX_TASKS) {
        if(edf_tasks[i].task_handle != NULL) {
            vTaskPrioritySet(edf_tasks[i].task_handle, EDF_UNSELECTED_PRIORITY);
        }

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
        printf("%d=", i);
        if(edf_tasks[i].task_state != EDF_TASK_SUSPENDED) {
            printf("%d\t", ((edf_tasks[i].task_deadline) - current_time)*portTICK_PERIOD_MS);
        }
        else {
            printf("N/A\t");
        }
    }
    printf(", %d\n", current_task_idx);
}

// Helper function that adds a task to the tasklist
static void add_task(TaskHandle_t task_handle, TickType_t task_deadline, TickType_t task_period, bool is_suspended) {
    // Do nothing if the maximum number of tasks have been reached
    if(num_tasks >= EDF_MAX_TASKS) {
        return;
    }
    
    // Add task to the back of the task array
    edf_tasks[num_tasks].task_handle = task_handle;
    edf_tasks[num_tasks].task_deadline = xTaskGetTickCount() + task_deadline;
    edf_tasks[num_tasks].task_period = task_period;
    edf_tasks[num_tasks].task_state = is_suspended ? EDF_TASK_SUSPENDED : EDF_TASK_READY;
    num_tasks++;
}

// Helper function that starts a task in the tasklist
static void start_task(uint8_t task_idx) {
    // Change task state using FreeRTOS
    vTaskResume(edf_tasks[task_idx].task_handle);

    // Update task status in scheduler
    edf_tasks[task_idx].task_state = EDF_TASK_READY;
}

// Helper function that suspends a task in the tasklist
static void suspend_task(uint8_t task_idx) {
    // Change task state using FreeRTOS
    vTaskSuspend(edf_tasks[task_idx].task_handle);

    // Update task status in scheduler
    edf_tasks[task_idx].task_state = EDF_TASK_SUSPENDED;
}

// Helper function that deletes a task from the tasklist
static void delete_task(uint8_t task_idx) {
    // Do nothing if there are no tasks or the requested task to remove is invalid
    if(num_tasks == 0 || task_idx >= num_tasks) {
        return;
    }

    // Delete the task using FreeRTOS
    vTaskDelete(edf_tasks[task_idx].task_handle);

    // Shift tasks one position to the left starting from the removed task's position
    for (uint8_t i = task_idx; i < num_tasks - 1; i++) {
        edf_tasks[i] = edf_tasks[i + 1];
    }

    // Remove task from array
    reset_task(num_tasks - 1);
    num_tasks--;

    // Update current_task_idx as necessary to reflect new tasklist
    if(task_idx == current_task_idx) {
        current_task_idx = -1;
    }
    else if(task_idx < current_task_idx) {
        current_task_idx--;
    }
}

// Mark a task in the tasklist as completed
// Returns the wakeup time for the next period if periodic, otherwise 0
static void complete_task(uint8_t task_idx) {
    // Will tenatively mark tasks that finish late as acceptable, but may have to handle this seperately in the future

    // Add periodic task back into the tasklist
    if(edf_tasks[task_idx].task_period > 0) {
        // Update deadline for next period
        edf_tasks[task_idx].task_deadline += edf_tasks[task_idx].task_period;
    }
    else {
        // Remove finished non-periodic task
        delete_task(task_idx);
    }
}

// Helper function that checks for and handles any missed deadlines
static void handle_missed_deadlines() {
    TickType_t current_time = xTaskGetTickCount();

    for(uint8_t i=0; i<num_tasks; i++) {
        // Determine whether deadlines have been missed
        if (current_time > edf_tasks[i].task_deadline) {
            printf("Task %d missed deadline (%d > %d, period=%d)\n", i, current_time*portTICK_PERIOD_MS, edf_tasks[i].task_deadline*portTICK_PERIOD_MS, edf_tasks[i].task_period*portTICK_PERIOD_MS);

            // If the missed task is periodic, skip and move onto the current period
            if(edf_tasks[i].task_period > 0) {
                while(edf_tasks[i].task_deadline < current_time) {
                    edf_tasks[i].task_deadline += edf_tasks[i].task_period;
                }
            }
            // If the missed task is not periodic, remove it from the tasklist (I believe this is safe because current_task_idx and num_tasks are dynamically adjusted)
            else {
                delete_task(i);
            }
        }
    }
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
static void switch_task(int8_t next_task_idx) {
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
static void edf_schedule() {
    // No tasks to schedule
    if (num_tasks == 0) {
        return;
    }

    // Address any missed deadlines first before doing any scheduling
    handle_missed_deadlines();

    // Determine the next task to switch to (if any) and switch to it
    int8_t next_task_idx = get_next_task_idx();
    switch_task(next_task_idx);

    // Update current task being run
    current_task_idx = next_task_idx;

    //print_deadlines();
}

static void edf_scheduler_task(void *pvParameters) {
    edf_scheduler_action_t received_action;
    
    while(1) {
        // Wait indefinitely for the queue (signal to perform an action on the tasklist)
        if(xQueueReceive(scheduler_action_queue, &received_action, portMAX_DELAY) == pdTRUE) {
            int8_t task_idx = find_task_idx(received_action.task_handle);

            switch (received_action.action_type) {
                case EDF_SCHEDULER_ACTION_ADD_TASK:
                    if(task_idx == -1) {
                        add_task(received_action.task_handle, received_action.task_deadline, received_action.task_period, received_action.is_suspended);
                        edf_schedule();
                    }
                    break;
                case EDF_SCHEDULER_ACTION_START_TASK:
                    if(task_idx != -1) {
                        start_task(task_idx);
                        edf_schedule();
                    }
                    break;
                case EDF_SCHEDULER_ACTION_SUSPEND_TASK:
                    if(task_idx != -1) {
                        suspend_task(task_idx);
                        edf_schedule();
                    }
                    break;
                case EDF_SCHEDULER_ACTION_DELETE_TASK:
                    if(task_idx != -1) {
                        delete_task(task_idx);
                        edf_schedule();
                    }
                    break;
                case EDF_SCHEDULER_ACTION_COMPLETE_TASK:
                    if(task_idx != -1) {
                        complete_task(task_idx);
                        edf_schedule();
                    }
                    break;
            }
        }
    }
}

// Start the EDF scheduler with an initial tasklist
// Will reject tasks that are not in a ready or suspended state 
bool edf_start(edf_task_t tasklist[], uint8_t tasklist_length) {
    // Indicate failure to receive tasklist (invalid tasklist array)
    if(tasklist == NULL) {
        return false;
    }
    // Indicate failure to receive tasklist (too many tasks)
    if(tasklist_length > EDF_MAX_TASKS) {
        return false;
    }

    // Initialize global objects
    num_tasks = 0;
    scheduler_action_queue = xQueueCreate(EDF_SCHEDULER_ACTION_QUEUE_LENGTH, sizeof(edf_scheduler_action_t));
    scheduler_started = true;

    // Reset all tasks first to guarantee default state
    for(uint8_t i=0; i < EDF_MAX_TASKS; i++) {
        reset_task(i);
    }

    // Add valid tasks from given tasklist to scheduler
    // Ignore any tasks that aren't in a ready or suspended state to avoid errors
    for(uint8_t i = 0; i < tasklist_length; i++) {
        if(tasklist[i].task_state == EDF_TASK_READY) {
            add_task(tasklist[i].task_handle, tasklist[i].task_deadline, tasklist[i].task_period, false);
        }
        else if(tasklist[i].task_state == EDF_TASK_SUSPENDED) {
            add_task(tasklist[i].task_handle, tasklist[i].task_deadline, tasklist[i].task_period, true);
        }
    }

    // Perform an initial schedule to figure out which task to do first
    edf_schedule();

    // Create the scheduler task
    xTaskCreate(edf_scheduler_task, "EDF Scheduler Task", EDF_SCHEDULER_STACK_SIZE, NULL, EDF_SCHEDULER_PRIORITY, &scheduler_task_handle);

    // Start the scheduler
    vTaskStartScheduler();

    return true;
}

// Add a new task to the scheduler
// Returns true on success, false otherwise
bool edf_add_task(TaskHandle_t task_handle, TickType_t task_deadline, TickType_t task_period, bool is_suspended) {
    // Indicate failure to add new task (scheduler has not been started yet)
    if(!scheduler_started) {
        return false;
    }
    // Indicate failure to add new task (illogical input)
    if(task_handle == NULL || task_deadline == portMAX_DELAY || task_period == portMAX_DELAY) {
        return false;
    }

    // Create action request to scheduler task
    edf_scheduler_action_t action = 
    {
        .action_type = EDF_SCHEDULER_ACTION_ADD_TASK,
        .task_handle = task_handle,
        .task_deadline = task_deadline,
        .task_period = task_period,
        .is_suspended = is_suspended,
    };

    // Send action request to scheduler task and return result
    return (xQueueSend(scheduler_action_queue, &action, portMAX_DELAY) == pdPASS);
}

// Start a task in the scheduler
// Returns true on success, false otherwise
bool edf_start_task(TaskHandle_t task_handle) {
    // Indicate failure to start task (scheduler has not been started yet)
    if(!scheduler_started) {
        return false;
    }
    // Indicate failure to start task (illogical input)
    if(task_handle == NULL) {
        return false;
    }

    // Create action request to scheduler task
    edf_scheduler_action_t action = 
    {
        .action_type = EDF_SCHEDULER_ACTION_START_TASK,
        .task_handle = task_handle,
    };

    // Send action request to scheduler task and return result
    return (xQueueSend(scheduler_action_queue, &action, portMAX_DELAY) == pdPASS);
}

// Pause a task in the scheduler
// Returns true on success, false otherwise
bool edf_suspend_task(TaskHandle_t task_handle) {
    // Indicate failure to suspend task (scheduler has not been started yet)
    if(!scheduler_started) {
        return false;
    }
    // Indicate failure to suspend task (illogical input)
    if(task_handle == NULL) {
        return false;
    }

    // Create action request to scheduler task
    edf_scheduler_action_t action = 
    {
        .action_type = EDF_SCHEDULER_ACTION_SUSPEND_TASK,
        .task_handle = task_handle,
    };

    // Send action request to scheduler task and return result
    return (xQueueSend(scheduler_action_queue, &action, portMAX_DELAY) == pdPASS);
}

// Delete a task from the scheduler
bool edf_delete_task(TaskHandle_t task_handle) {
    // Indicate failure to delete task (scheduler has not been started yet)
    if(!scheduler_started) {
        return false;
    }
    // Indicate failure to delete task (illogical input)
    if(task_handle == NULL) {
        return false;
    }

    // Create action request to scheduler task
    edf_scheduler_action_t action = 
    {
        .action_type = EDF_SCHEDULER_ACTION_DELETE_TASK,
        .task_handle = task_handle,
    };

    // Send action request to scheduler task and return result
    return (xQueueSend(scheduler_action_queue, &action, portMAX_DELAY) == pdPASS);
}

// Mark a task as completed in the scheduler
// Assumes that this this function is called in the current task
bool edf_complete_task(TaskHandle_t task_handle) {   
    // Indicate failure to complete task (scheduler has not been started yet)
    if(!scheduler_started) {
        return false;
    }
    // Indicate failure to complete task (illogical input)
    if(task_handle == NULL) {
        return false;
    }

    // Create action request to scheduler task
    edf_scheduler_action_t action = 
    {
        .action_type = EDF_SCHEDULER_ACTION_COMPLETE_TASK,
        .task_handle = task_handle,
    };

    // Get wakeup time for periodic tasks (start of next period)
    uint8_t task_idx = find_task_idx(task_handle);
    TickType_t wakeup_time = edf_tasks[task_idx].task_deadline;

    // Send action request to scheduler task and return result
    bool rc = xQueueSend(scheduler_action_queue, &action, portMAX_DELAY) == pdPASS;

    // Delay until next period (or the scheduler task just deletes the task calling this function)
    vTaskDelay(wakeup_time - xTaskGetTickCount());

    return rc;
}