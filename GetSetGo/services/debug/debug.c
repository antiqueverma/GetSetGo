#include "debug.h"
#include <stdarg.h>
// Based on FreeRTOS
/*********************************************************
 * Suggestions:
 *  Consider supporting per-tag log level control instead of a single global debugLevelMax.
 *  Allow switching output dynamically to different channels (UART, RTT, file, etc.) at runtime by tagging logs with output types.
 * If logging from ISR is needed, consider splitting API (debugLogFromISR with xQueueSend with ISR variant).
 * Add a compile-time feature flag to remove logging entirely (#define DEBUG_LOG_EN 0) for release builds.
 *
 **********************************************************/
static serial_port_t            *debugPort = NULL;
static StreamBufferHandle_t     debugStreamHandle;
static SemaphoreHandle_t        debugLogMutex;
static TaskHandle_t             debugTaskHandle = NULL;
static uint64_t                 debugTagMask; // All enabled by default
static uint8_t                  debugLevelMax;
static bool                     debugReady = false;

#define DEBUG_LOG_ENABLE(tagId) (debugTagMask |= (1UL << (tagId)))
#define DEBUG_LOG_DISABLE(tagId) (debugTagMask &= ~(1UL << (tagId)))
#define DEBUG_LOG_IS_ENABLED(tagId) ((debugTagMask >> (tagId)) & 0x1)

// Private Function Prototypes
static void debugTask(void *arg);
static uint32_t getTimeStamp(void);

// User Accessible API
gsg_result_t DEBUG_Init(void)
{
    debugLevelMax = DEBUG_LEVEL_INFO;
    debugTagMask = UINT64_MAX;

    debugLogMutex = xSemaphoreCreateMutex();
    debugStreamHandle = xStreamBufferCreate(DEBUG_TX_BUFF_SIZE, 1);
    configASSERT(debugLogMutex != NULL);
    configASSERT(debugStreamHandle != NULL);
    
    xTaskCreate(debugTask, "DBG",
                DEBUG_TASK_STACK_SIZE, NULL,
                DEBUG_TASK_PRIORITY, &debugTaskHandle);
    configASSERT(debugTaskHandle != NULL); // We cant assert here because our assert works on debug module itself
    return GSG_SUCCESS;
}

gsg_result_t DEBUG_setPort(serial_port_t *port)
{
    if(port == NULL)
        return GSG_INVALID_ARG;
    
    xSemaphoreTake(debugLogMutex, portMAX_DELAY);
    debugPort = port;
    xSemaphoreGive(debugLogMutex);

    return GSG_SUCCESS;
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
    if (level >= DEBUG_LEVEL_VERBOSE)
        level = DEBUG_LEVEL_VERBOSE;
    debugLevelMax = level;
}
uint8_t DEBUG_LogLevelGet(void)
{
    return debugLevelMax;
}
static void debugTask(void *arg)
{
    uint8_t txBuffer[DEBUG_MSG_MAX_LEN];
    size_t len;
    debugReady = true;
    
    while (1)
    {
        len = xStreamBufferReceive(debugStreamHandle, txBuffer, sizeof(txBuffer), portMAX_DELAY);

        if(len > 0)
        {
            if(debugPort != NULL)
            {
                if(SER_acquirePort(debugPort, portMAX_DELAY) == GSG_SUCCESS)
                {
                    debugPort->sendData(debugPort->context, txBuffer, len, 100);
                    SER_releasePort(debugPort);
                }
            }
        }
    }
    vTaskDelete(NULL);
}

static uint32_t getTimeStamp(void)
{
    return xTaskGetTickCount();
}

void debugLog(debugTagId_t tagId, char level, char *tag, char *fmt, ...)
{
    configASSERT(debugReady == true);  // Do not print until debug thread is ready 

    if (DEBUG_LOG_IS_ENABLED(tagId) == 0)
        return;

    if (xSemaphoreTake(debugLogMutex, 10) != pdTRUE)
        return;
        
    char msgBuff[DEBUG_MSG_MAX_LEN];
    memset(msgBuff, 0x00, sizeof(msgBuff));
    uint16_t offset = 0;
    msgBuff[offset++] = '\r';    // New Line
    msgBuff[offset++] = '\n';  // New Line
    msgBuff[offset++] = level; // Log level
    msgBuff[offset++] = ' ';   // Separator
    #if (DEBUG_TIMESTAMP_EN)
    int len = snprintf(&msgBuff[offset], DEBUG_MSG_MAX_LEN - offset, "[%lu] ", getTimeStamp());
    if (len > 0)
        offset += len;
    #endif
    #if (DEBUG_TAG_EN == 1)
    if (tag != NULL)
    {
        int len = snprintf((char *)&msgBuff[offset], DEBUG_MSG_MAX_LEN - offset, "%s:", tag);
        if (len > 0)
            offset += len;
    }
    #endif
    // Format the variadic message
    if (fmt != NULL)
    {
        va_list args;
        va_start(args, fmt);
        int len = vsnprintf((char *)&msgBuff[offset], DEBUG_MSG_MAX_LEN - offset, fmt, args);
        va_end(args);
        if (len > 0)
            offset += len;
    }
    xStreamBufferSend(debugStreamHandle, msgBuff, offset, 0);
    xSemaphoreGive(debugLogMutex);
    
}
void debugLogRaw(char *msg)
{
    configASSERT(debugReady == true);  // Do not print until debug thread is ready 
    if(msg == NULL) return;

    if (xSemaphoreTake(debugLogMutex, 100) != pdTRUE)
        return;
    
    xStreamBufferSend(debugStreamHandle, msg, strlen(msg), 0);

    xSemaphoreGive(debugLogMutex);
}
