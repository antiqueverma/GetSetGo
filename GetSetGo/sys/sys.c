
#include "sys.h"

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
    xTaskCreate(sysTaskHandler, "SYS", 512, NULL, 1, NULL);
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

void vApplicationTickHook( void )
{
    sysUpTimeCtr++;
}

uint64_t SYS_getUpTimeMs(void)
{
    return sysUpTimeCtr;
}