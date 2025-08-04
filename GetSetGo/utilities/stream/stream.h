
#ifndef STREAM_H_
#define STREAM_H_
#include "gsg_base.h"

#define STREAM_MAX_PARAMETERS       10
#define STREAM_USE_VAR_PTR          1
#define STREAM_TASK_PRIORITY        osPriorityLow
#define STREAM_TASK_STACK_SIZE      KB_to_B(3)
#define STREAM_INTERVAL_MS          (500)

void Stream_Init(void);
#if (STREAM_USE_VAR_PTR == 1)
    void Stream_unregisterVarPtr(int32_t *ptr);
    uint8_t Stream_registerVarPtr(int32_t *ptr);
#else
    void Stream_setData(uint8_t index, int32_t value);
    void Stream_resetData(uint8_t index);
#endif

typedef void (*streamTxCallback_t)(char *data);
void Stream_setTxCallback(streamTxCallback_t callback);

void Stream_triggerData( void );

#endif // STREAM_H_