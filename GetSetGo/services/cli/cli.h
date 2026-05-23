
#ifndef CLI_H_
#define CLI_H_
#include "gsg_defs.h"
#include "services/debug/debug.h"

#define CLI_TASK_PRIORITY       2
#define CLI_TASK_STACK_SIZE     KB_2_B(3)
#define CLI_RX_BUFF_SIZE        128
#define CLI_MODULES_MAX         10
#define CLI_INPUT_LINE_SIZE     128

#define CLI_MODULE_NAME_SIZE    10


// Type definitions
typedef void (*cli_cmd_callback_t)(const char *args);

typedef struct {
    char command[CLI_MODULE_NAME_SIZE+1];        // module name, e.g. "help"
    cli_cmd_callback_t callback;    // A callback function with standard signature
    char description[30];
    uint16_t  flags;        // bitmask for options 
} cliModuleData_t;

// Function Prototypes
void CLI_Init( void );
uint8_t CLI_RegisterModule(const char *name, cli_cmd_callback_t callback, const char *description);
void CLI_CommandHandler(const char *commandLine);
void CLI_rxISR(char byte);
void CLI_rxIsrProxy(char byte);



void cliHelpCommandCallback(char *args);



#endif /* CLI_H_ */
