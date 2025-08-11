#ifndef DEBUG_H_
#define DEBUG_H_

#include "gsg_base.h"

// Debugger Configs
#define DEBUG_LOG_EN                1
#define DEBUG_TASK_PRIORITY         (osPriority_t) osPriorityLow//1
#define DEBUG_TASK_STACK_SIZE       KB_to_B(2)
#define DEBUG_TX_BUFF_SIZE          KB_to_B(1)
#define DEBUG_MSG_MAX_LEN           128
#define DEBUG_TAG_EN                1
#define DEBUG_TIMESTAMP_EN          1       // 0:Disable, 1:10ms, 2: 100ms, 3:1000ms 
// Debugger Macros
typedef enum {
   DEBUG_LEVEL_NONE,
   DEBUG_LEVEL_ERR,
   DEBUG_LEVEL_WARN,
   DEBUG_LEVEL_INFO,
   DEBUG_LEVEL_DEBUG,
   DEBUG_LEVEL_VERBOSE,
} debugLevel_t;


typedef enum {
	DEBUG_TAG_RANDOM = 0,
   DEBUG_TAG_APP ,
   DEBUG_TAG_COMM ,
   DEBUG_TAG_SENSOR,
   DEBUG_TAG_SD ,
   DEBUG_TAG_UI ,


   // GSG MODULES
   DEBUG_TAG_MODBUS,
   _DEBUG_TAG_MAX
} debugTagId_t;

typedef enum {
    DEBUG_CHANNEL_DEFAULT = 0,
    DEBUG_CHANNEL_UART,
    DEBUG_CHANNEL_USB,
    DEBUG_CHANNEL_TCP,
    DEBUG_CHANNEL_UDP,
    DEBUG_CHANNEL_FILE,
    DEBUG_CHANNEL_SPI,
    DEBUG_CHANNEL_I2C,
    _DEBUG_CHANNEL_MAX
} debug_channel_t;

#if DEBUG_LOG_EN
   // Debugger User-API
   #define DEBUG_LOGE(id,tag,msg) \
      if (DEBUG_LogLevelGet() >= DEBUG_LEVEL_ERR) { debugLog(id, 'E', tag, msg); }
   #define DEBUG_LOGW(id,tag,msg) \
      if (DEBUG_LogLevelGet() >= DEBUG_LEVEL_WARN) { debugLog(id, 'W', tag, msg); }
   #define DEBUG_LOGI(id,tag,msg) \
      if (DEBUG_LogLevelGet() >= DEBUG_LEVEL_INFO) { debugLog(id, 'I', tag, msg); }
   #define DEBUG_LOGD(id,tag,msg) \
      if (DEBUG_LogLevelGet() >= DEBUG_LEVEL_DEBUG) { debugLog(id, 'D', tag, msg); }
   #define DEBUG_LOGV(id,tag,msg) \
      if (DEBUG_LogLevelGet() >= DEBUG_LEVEL_VERBOSE) { debugLog(id, 'V', tag, msg); }

   #define DEBUG_LOG_RAW(msg) \
      if (DEBUG_LogLevelGet() >= 0) { debugLogRaw(msg); }
   #define DEBUG_ASSERT(condition)                                      \
      do {                                                             \
         if (!(condition)) {                                          \
               debugLog(0, 'A', __FILE__, "ASSERT");           \
         }                                                            \
      } while (0)
    // osKernelLock();
    
#else
   // In release builds: compile to a clean semicolon
   #define DEBUG_LOGE(msg)                 ;
   #define DEBUG_LOGW(msg)                 ;
   #define DEBUG_LOGI(msg)                 ;
   #define DEBUG_LOG_RAW(msg)              ;
   #define DEBUG_ASSERT(condition)         ;
#endif
typedef void (*debugTxCallback_t)(const char *data, uint16_t size);
void DEBUG_Init( void );
void DEBUG_Log_Switch(debugTagId_t tag, bool enable);
void DEBUG_LogLevelSet(uint8_t level);
uint8_t DEBUG_LogLevelGet( void );
void debugLog(debugTagId_t tagId, char level, char * tag, char * msg);
void debugLogRaw(char * msg);
gsg_result_t DEBUG_RegisterTxCallback(debug_channel_t channel, debugTxCallback_t cb);
void DEBUG_setOutputChannel(uint8_t channel);
uint8_t DEBUG_getOutputChannel(void);
#endif

