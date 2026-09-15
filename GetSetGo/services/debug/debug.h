#ifndef DEBUG_H_
#define DEBUG_H_

#include "gsg_defs.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "stream_buffer.h"
#include <stdarg.h>
#include <stdio.h>
#include "services/serial/serial.h"

// Debugger Configs
#define DEBUG_LOG_EN                1
#define DEBUG_TASK_PRIORITY         2
#define DEBUG_TASK_STACK_SIZE       512
#define DEBUG_TX_BUFF_SIZE          KB_to_B(2)
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
   DEBUG_TAG_DEFAULT = 0,
   DEBUG_TAG_SYS,
   DEBUG_TAG_SVAR,
   DEBUG_TAG_MODBUS,
   DEBUG_TAG_CAN,
   DEBUG_TAG_COMM,
   DEBUG_TAG_SENSOR,
   DEBUG_TAG_FLASH,
   DEBUG_TAG_SD,
   DEBUG_TAG_UI,
   DEBUG_TAG_ESP,
   DEBUG_TAG_ASSERT,
   DEBUG_TAG_BSP,
   DEBUG_TAG_PSP,
   DEBUG_TAG_APP  // This mst be the last entry
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
      do { if (DEBUG_LogLevelGet() >= DEBUG_LEVEL_ERR) { debugLog(id, 'E', tag, __VA_ARGS__); } } while (0)
   #define DEBUG_LOGW(id,tag,...) \
      do { if (DEBUG_LogLevelGet() >= DEBUG_LEVEL_WARN) { debugLog(id, 'W', tag, __VA_ARGS__); } } while (0)
   #define DEBUG_LOGI(id,tag,...) \
      do { if (DEBUG_LogLevelGet() >= DEBUG_LEVEL_INFO) { debugLog(id, 'I', tag, __VA_ARGS__); } } while (0)
   #define DEBUG_LOGD(id,tag,...) \
      do { if (DEBUG_LogLevelGet() >= DEBUG_LEVEL_DEBUG) { debugLog(id, 'D', tag, __VA_ARGS__); } } while (0)
   #define DEBUG_LOGV(id,tag,...) \
      do { if (DEBUG_LogLevelGet() >= DEBUG_LEVEL_VERBOSE) { debugLog(id, 'V', tag, __VA_ARGS__); } } while (0)

   #define DEBUG_LOG_RAW(...) \
      do { debugLogRaw(__VA_ARGS__); } while (0)
   #define DEBUG_ASSERT(condition)                                                                 	\
      do {                                                                                         	\
         if (!(condition)) {                                                                       	\
               char assertStr[100];                                                                   \
               debugLogRaw("\n**********************************************************");          	\
               snprintf(assertStr, sizeof(assertStr), "\nAssert Failed in %s", __FILE__);              \
               debugLogRaw("%s", assertStr);                                                          \
               snprintf(assertStr, sizeof(assertStr), "\nLine: %d\n",__LINE__);                       \
               debugLogRaw("%s", assertStr);                                                          \
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
   #define DEBUG_LOGE(...)                 ;
   #define DEBUG_LOGW(...)                 ;
   #define DEBUG_LOGI(...)                 ;
   #define DEBUG_LOGD(...)                 ;
   #define DEBUG_LOGV(...)                 ;
   #define DEBUG_LOG_RAW(...)              ;
   #define DEBUG_ASSERT(condition)         ;
#endif
typedef void (*debugTxCallback_t)(const char *data, uint16_t size);
extern uint16_t   DEBUG_command;

gsg_result_t DEBUG_Init( void );
void DEBUG_Log_Switch(debugTagId_t tag, bool enable);
void DEBUG_LogLevelSet(uint8_t level);
uint8_t DEBUG_LogLevelGet( void );
void debugLog(debugTagId_t tagId, char level, const char *tag, const char *fmt, ...);
void debugLogRaw(const char *fmt, ...);
gsg_result_t DEBUG_RegisterTxCallback(debug_channel_t channel, debugTxCallback_t cb);
void DEBUG_setOutputChannel(debug_channel_t channel);
debug_channel_t DEBUG_getOutputChannel(void);
gsg_result_t DEBUG_setPort(serial_port_t *port);
#endif

