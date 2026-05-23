#ifndef DEBUG_H_
#define DEBUG_H_

#include "gsg_defs.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include <stdarg.h>

// Debugger Configs
#define DEBUG_LOG_EN                1
#define DEBUG_TASK_PRIORITY         11
#define DEBUG_TASK_STACK_SIZE       KB_to_B(2)
#define DEBUG_TX_BUFF_SIZE          KB_to_B(1)
#define DEBUG_MSG_MAX_LEN           128
#define DEBUG_TAG_EN                1
#define DEBUG_TIMESTAMP_EN          CONFIG_DEBUG_TIMESTAMP_ENABLE       // 0:Disable, 1:10ms, 2: 100ms, 3:1000ms 
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
   DEBUG_TAG_BSP ,
   DEBUG_TAG_MSP ,
   DEBUG_TAG_COMM ,
   DEBUG_TAG_SENSOR,
   DEBUG_TAG_SD ,
   DEBUG_TAG_UI ,

   // GSG MODULES
   DEBUG_TAG_MODBUS,
   DEBUG_TAG_ESP,
   DEBUG_TAG_ASSERT,
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
   #define DEBUG_LOGE(id,tag,...) \
      if (DEBUG_LogLevelGet() >= DEBUG_LEVEL_ERR) { debugLog(id, 'E', tag, __VA_ARGS__); }
   #define DEBUG_LOGW(id,tag,...) \
      if (DEBUG_LogLevelGet() >= DEBUG_LEVEL_WARN) { debugLog(id, 'W', tag, __VA_ARGS__); }
   #define DEBUG_LOGI(id,tag,...) \
      if (DEBUG_LogLevelGet() >= DEBUG_LEVEL_INFO) { debugLog(id, 'I', tag, __VA_ARGS__); }
   #define DEBUG_LOGD(id,tag,...) \
      if (DEBUG_LogLevelGet() >= DEBUG_LEVEL_DEBUG) { debugLog(id, 'D', tag, __VA_ARGS__); }
   #define DEBUG_LOGV(id,tag,...) \
      if (DEBUG_LogLevelGet() >= DEBUG_LEVEL_VERBOSE) { debugLog(id, 'V', tag, __VA_ARGS__); }

   #define DEBUG_LOG_RAW(msg) \
      if (DEBUG_LogLevelGet() >= 0) { debugLogRaw(msg); }
   #define DEBUG_ASSERT(condition)                                                                 	\
      do {                                                                                         	\
         if (!(condition)) {                                                                       	\
               char assertStr[100];                                                                   \
               debugLogRaw("\n**********************************************************");          	\
               sprintf(assertStr, "\nAssert Failed in %s", __FILE__);           						      \
               debugLogRaw(assertStr);                                                             	\
               sprintf(assertStr, "\nLine: %d\n",__LINE__);           								         \
               debugLogRaw(assertStr);                                                             	\
               debugLogRaw("**********************************************************");          	\
               if (xPortIsInsideInterrupt())                                                          \
               {                                                                                      \
                  taskDISABLE_INTERRUPTS();                                                           \
                  for(;;);                                                                            \
               }                                                                                      \
               else                                                                                   \
                  vTaskSuspend(NULL);                                                                 \
         }                                                                                         	\
      } while (0)
    
#else
   // In release builds: compile to a clean semicolon
   #define DEBUG_LOGE(msg)                 ;
   #define DEBUG_LOGW(msg)                 ;
   #define DEBUG_LOGI(msg)                 ;
   #define DEBUG_LOG_RAW(msg)              ;
   #define DEBUG_ASSERT(condition)         ;
#endif
typedef void (*debugTxCallback_t)(const char *data, uint16_t size);
extern uint16_t   DEBUG_command;

void DEBUG_Init( void );
void DEBUG_Log_Switch(debugTagId_t tag, bool enable);
void DEBUG_LogLevelSet(uint8_t level);
uint8_t DEBUG_LogLevelGet( void );
void debugLog(debugTagId_t tagId, char level, char *tag, char *fmt, ...);
void debugLogRaw(char * msg);
gsg_result_t DEBUG_RegisterTxCallback(debug_channel_t channel, debugTxCallback_t cb);
void DEBUG_setOutputChannel(debug_channel_t channel);
debug_channel_t DEBUG_getOutputChannel(void);
#endif

