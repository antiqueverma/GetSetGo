#include "gsg_config.h"
#if (defined(GSG_USE_STREAM) && (GSG_USE_STREAM == GSG_ENABLE))

#include "stream.h"

#if (STREAM_USE_VAR_PTR == 1)
    int32_t *streamVarPtrs[STREAM_MAX_PARAMETERS];
#else
    int32_t streamVars[STREAM_MAX_PARAMETERS];
#endif

static TaskHandle_t streamTaskHandle = NULL;
static osThreadId_t              streamTaskHandle = NULL;
static const osThreadAttr_t      streamTask_attributes = {
                                   .name = "STREAM",
                                   .stack_size = STREAM_TASK_STACK_SIZE,
                                   .priority = STREAM_TASK_PRIORITY,
                               };
static streamTxCallback_t    streamTxCallback = NULL;


static void sendStreamData(char *data);

static void streamTask( void *arg)
{
    char dataBuff[100] = {0};
    char temp[15] = {0};
    uint32_t notificationValue = 0;

    for(uint8_t i = 0; i < STREAM_MAX_PARAMETERS; i++)
    {
        #if (STREAM_USE_VAR_PTR == 1)
            streamVarPtrs[i] = NULL;    
        #else
            streamVars[i] = INT32_MAX;
        #endif
    }

    while(1)
    {
        uint8_t dataLength = 0;
        // Wait for a notification from the PLC task
        // if(xTaskNotifyWait(0, 0, NULL, portMAX_DELAY) == pdTRUE)
        {
            // Append the prefix
            sprintf(dataBuff, "$");
            // Append the stream variables
            for(uint8_t i = 0; i < STREAM_MAX_PARAMETERS; i++)
            {
                int32_t value = 0;

                #if (STREAM_USE_VAR_PTR == 1)
                    if(streamVarPtrs[i] != NULL)
                        value = *(streamVarPtrs[i]);
                    else
                        continue;
                #else
                    if(streamVars[i] < INT32_MAX)
                        value = streamVars[i];
                    else    // empty slot found
                        continue;
                #endif

                sprintf(temp,"%d",value);
                strcat(dataBuff, temp);
                strcat(dataBuff, " ");
            }
            // Append the suffix to the string
            dataLength = strlen(dataBuff);

            if(dataLength > 1)
            {
                dataBuff[dataLength - 1] = '\0';// Remove the last space and add a semicolon
                strcat(dataBuff, ";");
            }
            else if(dataLength == 1)  // there was no variable to stream
            {
                strcat(dataBuff, "0;");
            }
            
            if(streamTxCallback != NULL)
            {
                streamTxCallback(dataBuff); // Send the data using the callback
            }
            memset(dataBuff, 0, sizeof(dataBuff)); // Clear the data buffer
            vTaskDelay(pdMS_TO_TICKS(STREAM_INTERVAL_MS)); // Delay for the specified interval
        }
    }
    vTaskDelete(NULL); // Exit the thread gracefully
}

void Stream_Init(void)
{
    streamTaskHandle = osThreadNew(streamTask, NULL, &streamTask_attributes);
}

#if (STREAM_USE_VAR_PTR == 1)
// Save the external variable pointer to our stream variable list
uint8_t Stream_registerVarPtr(int32_t *ptr)
{
    // Check if the pointer is already registered
    for(uint8_t i = 0; i < STREAM_MAX_PARAMETERS; i++)
    {
        if(streamVarPtrs[i] == ptr)
        {
            // Pointer is already registered
            return 0;
        }
    }

    // Otherwise, find an empty slot
    for(uint8_t i = 0; i < STREAM_MAX_PARAMETERS; i++)
    {
        if(streamVarPtrs[i] == NULL)
        {
            streamVarPtrs[i] = ptr;
            return 1;
        }
    }
    return 0;
}
void Stream_unregisterVarPtr(int32_t *ptr)
{
    for(uint8_t i = 0; i < STREAM_MAX_PARAMETERS; i++)
    {
        // If the pointer is found, unregister it
        if(streamVarPtrs[i] == ptr)
        {
            streamVarPtrs[i] = NULL;
            break;
        }
    }
}
#else
void Stream_setData(uint8_t index, int32_t value)
{
    streamVars[index] = value;
}
void Stream_resetData(uint8_t index)
{
    streamVars[index] = INT32_MAX;
}
#endif

void Stream_triggerData( void )
{
    extern TaskHandle_t streamTaskHandle;

    // Notify the stream task with the new data
    if(streamTaskHandle != NULL)
    {
        xTaskNotify(streamTaskHandle, 0, eNoAction); // Notify the stream task to send data
    }
}

void Stream_setTxCallback(streamTxCallback_t callback)
{
    if(callback == NULL)
        return;
    streamTxCallback = callback;
}

#endif