#ifndef EDF_SCHEDULER_H
#define EDF_SCHEDULER_H

#include <FreeRTOS.h>
#include <task.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    TaskHandle_t taskHandle;
    TickType_t deadline;
} EDF_Task;

void EDF_Init(void);
void EDF_RegisterTask(EDF_Task *task, TickType_t deadline);
void EDF_StartScheduler(void);

#ifdef __cplusplus
}
#endif

#endif // EDF_SCHEDULER_H