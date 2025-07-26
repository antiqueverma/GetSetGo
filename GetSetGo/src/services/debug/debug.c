#include "debug.h"

// static uint8_t  buffer[DEBUG_TX_BUFF_SIZE] ;
static QueueHandle_t        debugTxQueue;
static SemaphoreHandle_t    debugLogMutex;
// static uint32_t             timer;
static uint64_t             debugTagMask; // All enabled by default
static uint8_t              debugLevelMax;  

#define DEBUG_LOG_ENABLE(tagId)     (debugTagMask |=  (1UL << (tagId)))    
#define DEBUG_LOG_DISABLE(tagId)    (debugTagMask &= ~(1UL << (tagId)))
#define DEBUG_LOG_IS_ENABLED(tagId) ((debugTagMask >> (tagId)) & 0x1)

// Private Function Prototypes
static void debugTransmitByte(char byte);
static void debugTask(void *arg);
static uint32_t getTimeStamp( void );

// User Accessible API
void DEBUG_Init( void )
{
    debugLevelMax = DEBUG_LEVEL_INFO;
    debugTagMask = UINT64_MAX;
    xTaskCreate(debugTask, "DBG", 2048, NULL, DEBUG_TASK_PRIORITY, NULL);
}

void DEBUG_Log_Switch(debugTagId_t tag, bool enable)
{
    if (enable)
        DEBUG_LOG_ENABLE(tag); 
    else
        DEBUG_LOG_DISABLE(tag);
}

void DEBUG_LogLevelSet(uint8_t level)
{
    if(level >= DEBUG_LEVEL_INFO)   level = DEBUG_LEVEL_INFO;
    debugLevelMax = level;
}

uint8_t DEBUG_LogLevelGet( void )
{
    return debugLevelMax;
}

// Library Internal Functions
#if 1

static void debugTask(void *arg)
{
    debugLogMutex = xSemaphoreCreateMutex();
    debugTxQueue = xQueueCreate(DEBUG_TX_BUFF_SIZE, sizeof(char));
    if (debugLogMutex == NULL || debugTxQueue == NULL)
    {
        // Initialization failed, handle as needed
        vTaskDelete(NULL);
    }

    char byte;
    while (1) 
    {
        if (xQueueReceive(debugTxQueue, &byte, portMAX_DELAY) == pdPASS)
        {
            debugTransmitByte(byte);
        }
    }

    vTaskDelete(NULL);
}

static void debugTransmitByte(char byte)
{
    #if (DEBUG_CHANNEL == 1)
        ;
    #elif (DEBUG_CHANNEL == 2)
        ;
    #else //Transmit over UART
        uart_write_bytes(DEBUG_UART_PORT, &byte, 1);
    #endif
}

static uint32_t getTimeStamp( void )
{
    return  xTaskGetTickCount();
}

void debugLog(debugTagId_t tagId, char level, char * tag, char * msg)
{
    if(DEBUG_LOG_IS_ENABLED(tagId) == 0)
        return;

    if (xSemaphoreTake(debugLogMutex, portMAX_DELAY) == pdTRUE)
    {
        char msgBuff[DEBUG_MSG_MAX_LEN];
        memset(msgBuff, 0x00, sizeof(msgBuff));
        uint16_t offset = 0;

        msgBuff[offset]     = '\r';      // New Line
        msgBuff[offset++]   = '\n';      // New Line
        msgBuff[offset++]   = level; // Log level
        msgBuff[offset++]   = ' ';   // Separator


        #if (DEBUG_TIMESTAMP_EN)
            int len = snprintf(&msgBuff[offset], DEBUG_MSG_MAX_LEN - offset, "[%u] ", getTimeStamp());
            if (len > 0) offset += len;
        #endif

        #if (DEBUG_TAG_EN == 1)
            if (tag != NULL) {
                int len = snprintf((char *)&msgBuff[offset], DEBUG_MSG_MAX_LEN - offset, "%s:", tag);
                if (len > 0) offset += len;
            }
        #endif

        if (msg != NULL) 
        {
            int len = snprintf((char *)&msgBuff[offset], DEBUG_MSG_MAX_LEN - offset, "%s", msg);
            if (len > 0) offset += len;
        }

        // Send each byte to the queue
        for (uint16_t i = 0; i < offset; i++)
        {
            if (xQueueSend(debugTxQueue, &msgBuff[i], 0) != pdPASS)
            {
                break;
            }
        }
        xSemaphoreGive(debugLogMutex);
    }
}

void debugLogRaw(char * msg)
{
    if (xSemaphoreTake(debugLogMutex, portMAX_DELAY) == pdTRUE)
    {
        // Send each byte to the queue until null terminator in source string
        while(*msg != '\0')
        {
            if (xQueueSend(debugTxQueue, msg, 0) != pdPASS)
            {
                break;
            }
            msg++;
        }
        xSemaphoreGive(debugLogMutex);
    }
}

#endif





