#include "taskmgr.h"

static SemaphoreHandle_t taskmgr_mutex;
static taskmgr_task_t taskRegistry[TASKMGR_MAX_TASKS];
static void houseKeepingTask(void *args);

static bool _taskmgr_getLock(void)
{
    if(xTaskGetSchedulerState() != taskSCHEDULER_RUNNING)
        return true;

    return (xSemaphoreTake(taskmgr_mutex, portMAX_DELAY) == pdTRUE);
}

static bool _taskmgr_releaseLock(void)
{
    if(xTaskGetSchedulerState() != taskSCHEDULER_RUNNING)
        return true;

    return (xSemaphoreGive(taskmgr_mutex) == pdTRUE);
}

static void _taskmgr_timerCallback(TimerHandle_t timer)
{
    taskmgr_task_t *task = (taskmgr_task_t *)pvTimerGetTimerID(timer);

    if(task == NULL || task->function == NULL)
        return;

    task->function(task->args);

    if(task->periodicityMs == 0)
    {
        taskENTER_CRITICAL();

        task->enabled = 0;
        task->inUse = 0;
        task->timer = NULL;

        taskEXIT_CRITICAL();

        xTimerDelete(timer, 0);
    }
}

static uint16_t _taskmgr_roundPeriod(uint16_t periodMs)
{
    if(periodMs == 0)
        return 0;

    return (periodMs / 10U) * 10U;
}

gsg_result_t TASKMGR_Init(void)
{
    memset(taskRegistry, 0, sizeof(taskRegistry));

    taskmgr_mutex = xSemaphoreCreateMutex();

    if(taskmgr_mutex == NULL)
        return GSG_ERROR;

    return TASKMGR_createTask( houseKeepingTask, NULL, TASKMGR_HOUSEKEEPING_PERIOD_MS, TASKMGR_HOUSEKEEPING_TASK_ID);
}

static void houseKeepingTask(void *args)
{
    (void)args;

    _taskmgr_getLock();

    for(uint32_t i = 0; i < TASKMGR_MAX_TASKS; i++)
    {
        if(taskRegistry[i].inUse &&
           taskRegistry[i].pendingDelete)
        {
            xTimerDelete(taskRegistry[i].timer, portMAX_DELAY);

            memset(&taskRegistry[i], 0, sizeof(taskRegistry[i]));
        }
    }

    _taskmgr_releaseLock();
}

gsg_result_t TASKMGR_createTask(taskFunction_t function, void *args, uint16_t periodicityMs, uint16_t id)
{
    if(function == NULL)
        return GSG_INVALID_ARG;

    periodicityMs = _taskmgr_roundPeriod(periodicityMs);

    _taskmgr_getLock();

    for(uint32_t i = 0; i < TASKMGR_MAX_TASKS; i++)
    {
        if(taskRegistry[i].inUse == 0)
        {
            taskRegistry[i].function        = function;
            taskRegistry[i].args            = args;
            taskRegistry[i].periodicityMs   = periodicityMs;
            taskRegistry[i].id              = id;
            taskRegistry[i].inUse           = 1;
            taskRegistry[i].enabled         = 1;

            taskRegistry[i].timer =
                xTimerCreate( "TM", pdMS_TO_TICKS(periodicityMs == 0 ? 1 : periodicityMs), (periodicityMs != 0), &taskRegistry[i], _taskmgr_timerCallback);

            if(taskRegistry[i].timer == NULL)
            {
                memset(&taskRegistry[i], 0, sizeof(taskRegistry[i]));
                _taskmgr_releaseLock();
                return GSG_ERROR;
            }

            if(xTimerStart(taskRegistry[i].timer, 0) != pdPASS)
            {
                xTimerDelete(taskRegistry[i].timer, 0);
                memset(&taskRegistry[i], 0, sizeof(taskRegistry[i]));

                _taskmgr_releaseLock();
                return GSG_ERROR;
            }

            _taskmgr_releaseLock();
            return GSG_SUCCESS;
        }
        else if(taskRegistry[i].id == id)
        {
            // Task with the same ID already exists
            _taskmgr_releaseLock();
            return GSG_ERROR;
        }
    }

    _taskmgr_releaseLock();
    return GSG_ERROR;
}

gsg_result_t TASKMGR_deleteTask(uint16_t id)
{
    if(id == TASKMGR_HOUSEKEEPING_TASK_ID)
        return GSG_INVALID_ARG;

    _taskmgr_getLock();

    for(uint32_t i = 0; i < TASKMGR_MAX_TASKS; i++)
    {
        if(taskRegistry[i].inUse &&
           taskRegistry[i].id == id)
        {
            taskRegistry[i].pendingDelete = 1;
            taskRegistry[i].enabled = 0;

            xTimerStop(taskRegistry[i].timer, portMAX_DELAY);

            _taskmgr_releaseLock();

            return GSG_SUCCESS;
        }
    }

    _taskmgr_releaseLock();

    return GSG_ERROR;
}

gsg_result_t TASKMGR_enableTask(uint16_t id)
{
    _taskmgr_getLock();

    for(uint32_t i = 0; i < TASKMGR_MAX_TASKS; i++)
    {
        if(taskRegistry[i].inUse && taskRegistry[i].id == id)
        {
            if(xTimerStart(taskRegistry[i].timer, portMAX_DELAY) == pdPASS)
            {
                taskRegistry[i].enabled = 1;

                _taskmgr_releaseLock();
                return GSG_SUCCESS;
            }

            break;
        }
    }

    _taskmgr_releaseLock();
    return GSG_ERROR;
}

gsg_result_t TASKMGR_disableTask(uint16_t id)
{
    _taskmgr_getLock();

    for(uint32_t i = 0; i < TASKMGR_MAX_TASKS; i++)
    {
        if(taskRegistry[i].inUse && taskRegistry[i].id == id)
        {
            if(xTimerStop(taskRegistry[i].timer, portMAX_DELAY) == pdPASS)
            {
                taskRegistry[i].enabled = 0;

                _taskmgr_releaseLock();
                return GSG_SUCCESS;
            }

            break;
        }
    }

    _taskmgr_releaseLock();
    return GSG_ERROR;
}



gsg_result_t TASKMGR_updatePeriodicity(uint16_t id, uint16_t periodicityMs)
{
    periodicityMs = _taskmgr_roundPeriod(periodicityMs);

    _taskmgr_getLock();

    for(uint32_t i = 0; i < TASKMGR_MAX_TASKS; i++)
    {
        if(taskRegistry[i].inUse &&
           taskRegistry[i].id == id)
        {
            if(xTimerChangePeriod(
                    taskRegistry[i].timer,
                    pdMS_TO_TICKS(periodicityMs == 0 ? 1 : periodicityMs),
                    portMAX_DELAY) == pdPASS)
            {
                taskRegistry[i].periodicityMs = periodicityMs;

                _taskmgr_releaseLock();
                return GSG_SUCCESS;
            }

            break;
        }
    }

    _taskmgr_releaseLock();
    return GSG_ERROR;
}

gsg_result_t TASKMGR_getPeriodicity(uint16_t id, uint16_t *periodicityMs)
{
    if(periodicityMs == NULL)
        return GSG_INVALID_ARG;

    _taskmgr_getLock();

    for(uint32_t i = 0; i < TASKMGR_MAX_TASKS; i++)
    {
        if(taskRegistry[i].inUse &&
           taskRegistry[i].id == id)
        {
            *periodicityMs = taskRegistry[i].periodicityMs;

            _taskmgr_releaseLock();
            return GSG_SUCCESS;
        }
    }

    _taskmgr_releaseLock();
    return GSG_ERROR;
}