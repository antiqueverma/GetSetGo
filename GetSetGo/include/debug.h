
#ifndef DEBUG_H_
#define DEBUG_H_

#include "main.h"

// Debugger Configs
#define DEBUG_TASK_PRIORITY         1
#define DEBUG_TX_BUFF_SIZE          KB_2_B(1)
#define DEBUG_CHANNEL               0 // 0: Default UART, 1: Socket, 2: File
#define DEBUG_MSG_MAX_LEN           128
#define DEBUG_TAG_EN                1
#define DEBUG_TIMESTAMP_EN          1       // 0:Disable, 1:10ms, 2: 100ms, 3:1000ms  

// Debugger Macros
#define DEBUG_LEVEL_NONE            0
#define DEBUG_LEVEL_ERR             1
#define DEBUG_LEVEL_WARN            2
#define DEBUG_LEVEL_INFO            3

typedef enum {
    DEBUG_TAG_APP     = 0,
    DEBUG_TAG_COMM    = 1,
    DEBUG_TAG_SENSOR  = 2,
    DEBUG_TAG_SD      = 3,
    DEBUG_TAG_UI      = 4,

    _DEBUG_TAG_MAX
} debugTagId_t;

// Debugger User-API
#define DEBUG_LOGE(msg) \
    if (DEBUG_LogLevelGet() >= 1) { debugLog(debugTagId, 'E', debugTag, msg); }

#define DEBUG_LOGW(msg) \
    if (DEBUG_LogLevelGet() >= 2) { debugLog(debugTagId, 'W', debugTag, msg); }

#define DEBUG_LOGI(msg) \
    if (DEBUG_LogLevelGet() >= 3) { debugLog(debugTagId, 'I', debugTag, msg); }

#define DEBUG_LOG_RAW(msg) \
    if (DEBUG_LogLevelGet() >= 0) { debugLogRaw(msg); }

#define DEBUG_ASSERT()

void DEBUG_Init( void );
void DEBUG_Log_Switch(debugTagId_t tag, bool enable);
void DEBUG_LogLevelSet(uint8_t level);
uint8_t DEBUG_LogLevelGet( void );
void debugLog(debugTagId_t tagId, char level, char * tag, char * msg);
void debugLogRaw(char * msg);

#endif











