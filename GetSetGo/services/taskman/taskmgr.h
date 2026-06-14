

#ifndef TASKMGR_H_
#define TASKMGR_H_

#include "FreeRTOS.h"
#include "semphr.h"
#include "timers.h"
#include "gsg_defs.h"

#define TASKMGR_MAX_TASKS    16
#define TASKMGR_HOUSEKEEPING_PERIOD_MS    100
#define TASKMGR_HOUSEKEEPING_TASK_ID      0

typedef void (*taskFunction_t)(void *args);

typedef struct {
    taskFunction_t function;
    void *args;
    uint16_t periodicityMs;
    uint16_t id;
    TimerHandle_t timer;
    uint8_t inUse:1;
    uint8_t enabled:1;
    uint8_t pendingDelete:1;

    uint8_t __reserved:5;
} taskmgr_task_t;

gsg_result_t TASKMGR_Init(void);
gsg_result_t TASKMGR_createTask(taskFunction_t function, void *args, uint16_t periodicityMs, uint16_t id);
gsg_result_t TASKMGR_deleteTask(uint16_t id);
gsg_result_t TASKMGR_enableTask(uint16_t id);
gsg_result_t TASKMGR_disableTask(uint16_t id);

#endif /* TASKMGR_H_ */