#include "cli.h"



// Private Function Prototypes
static void cliTask(void *arg);


// Variables
extern cliModuleData_t cliDescrTable[];
extern uint8_t commandCount; // Total number of commands available in the table
// static uint8_t  buffer[DEBUG_TX_BUFF_SIZE] ;
static QueueHandle_t       cliRxQueue;
TaskHandle_t cliTaskHandle = NULL;
static char         debugTag[] = "CLI";
static debugTagId_t debugTagId = 1;


// User Accessible API
void CLI_Init( void )
{
    cliRxQueue = xQueueCreate(CLI_RX_BUFF_SIZE, sizeof(char));
    if(cliRxQueue == NULL)
    {
        DEBUG_LOGE("Failed RxQue Create");
        return; // Initialization failed, handle as needed
    }   
    
    xTaskCreate(cliTask, "CLI", CLI_TASK_STACK_SIZE, NULL, CLI_TASK_PRIORITY, &cliTaskHandle);
}

uint8_t CLI_RegisterModule(const char *name, cli_cmd_callback_t callback, const char *description)
{
    for (uint8_t i = 0; i < CLI_MODULES_MAX; i++)
    {
        if (cliDescrTable[i].command[0] == '\0') // empty slot
        {
            if (name == NULL || callback == NULL)
                return pdFALSE; // Invalid parameters


            strncpy(cliDescrTable[i].command, name, CLI_MODULE_NAME_SIZE);
            cliDescrTable[i].command[CLI_MODULE_NAME_SIZE] = '\0';

            if (description != NULL)
                strncpy(cliDescrTable[i].description, description, sizeof(cliDescrTable[i].description) - 1);
            
            cliDescrTable[i].callback = callback;
            commandCount++; // Increment the command count
            return pdTRUE; // Successfully registered the command
        }
    }
    return pdFALSE; // Invalid parameters
    DEBUG_LOGE("No space to register command");
}


void CLI_rxISR(char byte)
{
    // This function is called from the UART ISR to handle incoming bytes
    xQueueSendFromISR(cliRxQueue, &byte, NULL);
    if(byte == '\n' || byte == '\r')
    {
        // Notify the CLI task that a new line has been received
        vTaskNotifyGiveFromISR(cliTaskHandle, NULL);
    }
}

// This function is called from a high-priority task that behaves like an ISR, for cases when 
// ISR callback is not available
void CLI_rxIsrProxy(char byte)
{
    static uint16_t byteCount = 0;
    xQueueSend(cliRxQueue, &byte, 0);
    byteCount++;
    uint32_t notifVal = 0;

    if((byte == '\n' || byte == '\r')  )
    {   
        if(byteCount > 0)
        {
            byteCount = 0; // Reset byte count after a complete line is received
            // Also Notify the CLI task that a new line has been received
            xTaskNotify(cliTaskHandle, notifVal, eNoAction);
        }
        else // \r \n was received without any data, pop the queue to clear it 
        {
            xQueueReceive(cliRxQueue, &byte, 0); // Clear the previously inserted \r or \n 
        }
    }
}

void cliHelpCommandCallback(char *args)
{
    DEBUG_LOGI("Available Commands: ");
    // Print all available commands using DEBUG_LOGI
    for (uint16_t i = 0; i < commandCount; i++) 
    {
        if (cliDescrTable[i].command[0] != '\0') 
        {
            char hex[100];
            sprintf(hex,"\r\n\tCmd: %s - %s", cliDescrTable[i].command, cliDescrTable[i].description);
            DEBUG_LOG_RAW(hex);
        }
    }
}

static void cliTask(void *arg)
{
    char buffer[CLI_INPUT_LINE_SIZE + 1]; // Buffer to hold the command line
    uint16_t bufferIndex = 0;
    uint16_t i;
    // char hex[100];
    while (1) 
    {
        // wait for a notification that a new command line is available
        if(xTaskNotifyWait(0, 0, NULL, portMAX_DELAY) == pdTRUE)
        {
            bufferIndex = 0;
            while(xQueueReceive(cliRxQueue, buffer + bufferIndex, portMAX_DELAY) == pdTRUE  )
            {   
                bufferIndex++;
                if((bufferIndex > (sizeof(buffer) - 1)) || (buffer[bufferIndex - 1] == '\n') || (buffer[bufferIndex - 1] == '\r'))
                {
                    // Null-terminate the string, overwrite NULL character on \r or \n 
                    buffer[bufferIndex - 1] = '\0';
                    break; // Exit the loop when a complete command line is received
                }
            }

            //
            char moduleName[CLI_MODULE_NAME_SIZE+1];      // Get module name from command line into this buffer
            strtok(buffer, " "); // Split the command line by spaces
            strncpy(moduleName, buffer, CLI_MODULE_NAME_SIZE - 1);
            moduleName[CLI_MODULE_NAME_SIZE - 1] = '\0'; // Ensure null termination

            // Check the command against the registered commands, 
            // loop through the command table and find the matching command
            for(i = 0; i < commandCount; i++)
            {
                if(strncmp(cliDescrTable[i].command, moduleName, CLI_MODULE_NAME_SIZE) == 0)
                {
                    // Found the command in the table, call the callback function if it exists
                    if(cliDescrTable[i].callback != NULL)
                    {
                        cliDescrTable[i].callback(buffer + strlen(moduleName) + 1); // Pass the rest of the command line to the callback
                    }
                    else
                    {
                        // If no callback is registered, print the description if available
                        if(cliDescrTable[i].description != NULL)
                        {
                            DEBUG_LOGI("\r\t");
                            DEBUG_LOG_RAW(cliDescrTable[i].command)
                            DEBUG_LOG_RAW(" : ") ;
                            DEBUG_LOG_RAW(cliDescrTable[i].description);
                        }
                        else
                        {
                            DEBUG_LOGI("Cmd not available");
                        }
                    }
                    break; // Exit the for loop after finding the command
                }
            }
            if(i == commandCount) // If we reached the end of the table without finding the command
            {// Command not found, print an error message
                    DEBUG_LOGE("Cmd not found.");
            }
        }
        // parse first command and check if it exists in table
    }

    vTaskDelete(NULL);
}

void CLI_PrintVersion(void)
{
    // Print version information
    DEBUG_LOGI("CLI Version: 1.0.0");
    DEBUG_LOG_RAW("Build Date: " __DATE__ " " __TIME__);
    DEBUG_LOG_RAW("C Standard: C17");
}

// This function template can be used to implement a CLI command callback
// It can be customized based on the specific command requirements
void __cliCallback(char *args)
{
    uint8_t commandId = 0; // This can be used to identify the command if needed
    char *token;

    
    //save the token from args into token variable
    token = strtok(args, " ");
    if (token == NULL) 
    {
        DEBUG_LOGE("No command provided");
        return; // No command provided, handle as needed
    }

    if(strcmp(token, "command1") == 0)
    {
        commandId = 1; // Set command ID based on the command name
    }
    else if(strcmp(token, "command2") == 0)
    {
        commandId = 2; // Set command ID based on the command name
    }       
    else
    {
        DEBUG_LOGE("Unknown command");
        return; // Unknown command, handle as needed                            
    }
    
    switch( commandId ) // Replace with actual command ID or logic to determine the command
    {
        case 0:
        {
            // get the next token from args
            token = strtok(NULL, " ");
            if (token == NULL)
            {
                DEBUG_LOGE("No arguments provided for command1");
                return; // No arguments provided, handle as needed
            }
            // convert the token to an integer or perform the required operation
            int argValue = atoi(token); 
            DEBUG_LOGI("OK");
            break;
        }
        case 1:
        {
            break;
        }
        default:
        {
            DEBUG_LOGE("Invalid command");
            break;
        }
    }
}