
#ifndef STREAM_H_
#define STREAM_H_
#include "main.h"

#define STREAM_MAX_PARAMETERS       10
#define STREAM_USE_VAR_PTR          1
#define STREAM_TASK_PRIORITY        3
#define STREAM_TASK_STACK_SIZE      KB_to_B(3)
#define STREAM_INTERVAL_MS          pdMS_TO_TICKS(500)

void Stream_Init(void);
#if (STREAM_USE_VAR_PTR == 1)
    void Stream_unregisterVarPtr(int32_t *ptr);
    uint8_t Stream_registerVarPtr(int32_t *ptr);
#else
    void Stream_setData(uint8_t index, int32_t value);
    void Stream_resetData(uint8_t index);
#endif


void Stream_triggerData( void );

#endif // STREAM_H_