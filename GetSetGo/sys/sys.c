
#include "sys.h"

#include <stdio.h>
#include <string.h>

#include "gsg_mcu.h"

#include "gsg_base.h"

// Extern from main.c
extern UART_HandleTypeDef huart1;

enum {
    SYS_STATE_PSP_INIT,
    SYS_STATE_BSP_INIT,
    SYS_STATE_APP_INIT,
    SYS_STATE_RUNNING,
    SYS_STATE_ERROR,
} sys_state_t;

static uint64_t sysUpTimeCtr = 0;

static void sysTaskHandler(void *args);

void SYS_Init(void) 
{
    xTaskCreate(sysTaskHandler, "SYS", 256, NULL, 1, NULL);
}

extern void PSP_Init(void);
extern uint8_t PSP_getState(void);
extern void BSP_Init(void);
extern uint8_t BSP_getState(void);
extern void APP_Init(void);

static void sysTaskHandler(void *args)
{
    while (1)
    {
        switch (sys_state_t)
        {
            case SYS_STATE_PSP_INIT:
            {
                PSP_Init();
                if(PSP_getState() == 0)
                {   
                    sys_state_t = SYS_STATE_ERROR;
                    break;
                }
                sys_state_t = SYS_STATE_BSP_INIT;
                break;
            }

            case SYS_STATE_BSP_INIT:
            {
                BSP_Init();
                if(BSP_getState() == 0)
                {
                    DEBUG_LOGI(DEBUG_TAG_SYS, "SYS", "BSP Init Error");
                    sys_state_t = SYS_STATE_ERROR;
                    break;
                }
                sys_state_t = SYS_STATE_APP_INIT;
                break;
            }

            case SYS_STATE_APP_INIT:
            {
                APP_Init();
                sys_state_t = SYS_STATE_RUNNING;
                break;
            }

            case SYS_STATE_RUNNING:
            {
                vTaskDelay(1000); // Sleep for a while to reduce CPU usage
                break;
            }
            
            case SYS_STATE_ERROR:
            {
                DEBUG_LOGE(DEBUG_TAG_SYS, "SYS", "Error");
                vTaskDelay(1000); // Sleep for a while to reduce CPU usage
                break;
            } 
        }
    }
}

uint64_t SYS_getUpTimeMs(void)
{
    return sysUpTimeCtr;
}

/* FreeRTOS stack overflow hook.
 * Prints a diagnostic message over UART and halts the system.
 */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    char msg[128];
    const char *name = pcTaskName ? pcTaskName : "Unknown";
    int n = snprintf(msg, sizeof(msg), "*** Stack overflow in task: %s\r\n", name);
    if (n > 0)
    {
        uint16_t len = (uint16_t)((n >= (int)sizeof(msg)) ? (sizeof(msg) - 1) : n);
        HAL_UART_Transmit(&huart1, (uint8_t *)msg, len, HAL_MAX_DELAY);
    }
    __disable_irq();
    for(;;);
}

void vApplicationTickHook( void )
{
    sysUpTimeCtr++;

    // Get the freeRTOS tick count
    uint32_t currTickCount = xTaskGetTickCount();
    static uint32_t prevTickCount;

    if(prevTickCount == 0)
        prevTickCount = currTickCount;
    
    
    if((currTickCount - prevTickCount) > 1000)  // 1000ms elapsed
    {
        prevTickCount = currTickCount;
        #ifdef GSG_USE_RTC
            #if (GSG_USE_RTC == GSG_ENABLE)
            RTC_IncrementSeconds();
            #endif
        #endif
    }

}
