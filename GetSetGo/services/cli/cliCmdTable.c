#include "cli.h"

// Module Cmd Callbacks prototypes



//Module Command Table
cliModuleData_t cliCommands[] = 
{
    {"help",        cliHelpCommandCallback,         "Print all available commands", 0x00},
    {"version",     NULL,                           "Print version info", 0x00},
    {"reset",       NULL,                           "Reset MCU", 0x00},
    {"tasklist",    NULL,                           "Print task list", 0x00},
    {"debug",       NULL,                           "Debug command", 0x00},
    {"gpio",        NULL,                           "GPIO command", 0x00},
    // SVAR commands
    {"setVar",        NULL,                         "Set SVAR Value", 0x00},
    {"getVar",        NULL,                         "Get SVAR Value", 0x00},
};

uint8_t commandCount = sizeof(cliDescrTable) / sizeof(cliModuleData_t); // Total number of commands registered
