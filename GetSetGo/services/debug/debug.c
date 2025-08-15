#include "debug.h"
/*********************************************************
* Suggestions:
*  Consider supporting per-tag log level control instead of a single global debugLevelMax.
*  Allow switching output dynamically to different channels (UART, RTT, file, etc.) at runtime by tagging logs with output types.
* If logging from ISR is needed, consider splitting API (debugLogFromISR with osMessageQueuePut with ISR variant).
* Add a compile-time feature flag to remove logging entirely (#define DEBUG_LOG_EN 0) for release builds.
*
**********************************************************/
static osMessageQueueId_t        debugTxQueue;
// static StreamBufferHandle_t     debugStreamHandle;
static osMutexId_t               debugLogMutex;
static osThreadId_t              debugTaskHandle = NULL;
static const osThreadAttr_t      debugTask_attributes = {
                                   .name = "DBG",
                                   .stack_size = DEBUG_TASK_STACK_SIZE,
                                   .priority = DEBUG_TASK_PRIORITY,
                               };
static debugTxCallback_t    debugTxCallbacks[_DEBUG_CHANNEL_MAX] = {0};
static uint64_t             debugTagMask; // All enabled by default
static uint8_t              debugLevelMax;
static debug_channel_t      debugChannel = DEBUG_CHANNEL_DEFAULT;
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
   debugLevelMax = DEBUG_LEVEL_VERBOSE;
   debugTagMask = UINT64_MAX;
   debugTaskHandle = osThreadNew(debugTask, NULL, &debugTask_attributes);
   osDelay(5);
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
    if(level >= DEBUG_LEVEL_VERBOSE)   
        level = DEBUG_LEVEL_VERBOSE;
    debugLevelMax = level;
}
uint8_t DEBUG_LogLevelGet( void )
{
   return debugLevelMax;
}
static void debugTask(void *arg)
{
   debugLogMutex = osMutexNew(NULL);
   debugTxQueue = osMessageQueueNew(DEBUG_TX_BUFF_SIZE, sizeof(char), NULL);
   if (debugLogMutex == NULL || debugTxQueue == NULL)
   {
       // Initialization failed, delete the task
       osThreadExit();
    }
   char byte;
   while (1)
   {
       if (osMessageQueueGet(debugTxQueue, &byte, NULL, osWaitForever) == osOK)
       {
           debugTransmitByte(byte);
       }
   }
    osThreadExit();
}
static void debugTransmitByte(char byte)
{
   if(debugChannel >= _DEBUG_CHANNEL_MAX || debugTxCallbacks[debugChannel] == NULL)
       return; // No callback registered for this channel
  
   debugTxCallbacks[debugChannel](&byte, 1);
}
static uint32_t getTimeStamp( void )
{
   return  osKernelGetTickCount();
}
void debugLog(debugTagId_t tagId, char level, char * tag, char * msg)
{
   if(DEBUG_LOG_IS_ENABLED(tagId) == 0)
       return;
//   if (osMutexAcquire(debugLogMutex, 10) == osOK)
   {
       char msgBuff[DEBUG_MSG_MAX_LEN];
       memset(msgBuff, 0x00, sizeof(msgBuff));
       uint16_t offset = 0;
       msgBuff[offset]     = '\r';      // New Line
       msgBuff[offset++]   = '\n';      // New Line
       msgBuff[offset++]   = level; // Log level
       msgBuff[offset++]   = ' ';   // Separator
       #if (DEBUG_TIMESTAMP_EN)
           int len = snprintf(&msgBuff[offset], DEBUG_MSG_MAX_LEN - offset, "[%lu] ", getTimeStamp());
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
           if (osMessageQueuePut(debugTxQueue, &msgBuff[i], 0, 0) != osOK)
           {
               break;
           }
       }
//       osMutexRelease(debugLogMutex);
   }
}
void debugLogRaw(char * msg)
{
   if (osMutexAcquire(debugLogMutex, osWaitForever) == osOK)
   {
       // Send each byte to the queue until null terminator in source string
       while(*msg != '\0')
       {
           if (osMessageQueuePut(debugTxQueue, msg, 0, 0) != osOK)
           {
               break;
           }
           msg++;
       }
       osMutexRelease(debugLogMutex);
   }
}
gsg_result_t DEBUG_RegisterTxCallback(debug_channel_t channel, debugTxCallback_t cb)
{
   if ((channel >=  _DEBUG_CHANNEL_MAX) || cb == NULL)
       return GSG_INVALID_ARG;

   debugTxCallbacks[channel] = cb;
   return GSG_SUCCESS;
}
void DEBUG_setOutputChannel(debug_channel_t channel)
{
   if (channel < _DEBUG_CHANNEL_MAX)
   {
       debugChannel = channel;
   }
   else
   {
       // Handle invalid channel case, e.g., log an error or set to default
       debugChannel = DEBUG_CHANNEL_DEFAULT; // Default to first channel
   }
}
debug_channel_t DEBUG_getOutputChannel(void)
{
   return debugChannel;
}
