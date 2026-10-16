
#ifndef CBUS_APP_H_
#define CBUS_APP_H_

#include <stdint.h>

#define CBUS_TX_TASK_PRIORITY 8
#define CBUS_TX_TASK_STACK_SIZE 256
#define CBUS_TX_QUEUE_LENGTH 5

#define CBUS_RX_TASK_PRIORITY 9
#define CBUS_RX_TASK_STACK_SIZE 256
#define CBUS_RX_QUEUE_LENGTH 5

#define CBUS_PROCESS_TASK_PRIORITY 7
#define CBUS_PROCESS_TASK_STACK_SIZE 512

#define CBUS_MAX_DEVICES        10

#define CBUS_NODE_ID             1
#define CBUS_CAN_BITRATE         500
#define CBUS_HEARTBEAT_MS        500
#define CBUS_SDO_TIMEOUT_MS      1000

#define CBUS_USE_SVAR_DATABASE   1
// #define CBUS_DEV_SIM_ENABLED

typedef enum
{
    CBUS_DEV_STATE_UNREGISTERED,   // Device is not registered with the bus
    CBUS_DEV_STATE_INITIALIZING,   // Initializing hardware and software
    CBUS_DEV_STATE_BOOTING,        // Boot-up sequence in progress
    CBUS_DEV_STATE_IDLE,           // Ready and waiting for activity
    CBUS_DEV_STATE_BUSY,           // Processing an operation
    CBUS_DEV_STATE_COMM_ERROR,     // Communication failure
    CBUS_DEV_STATE_ERROR,          // Internal or unrecoverable error
    CBUS_DEV_STATE_DISCOVERABLE,   // Available for discovery
} cbus_dev_state_t;

typedef enum {
    FLAG1,
} cbus_dev_flags_t;

typedef struct {
    int32_t         value;
    uint8_t         precision;
    char            postfix[8];
} cbus_channel_data_t;

typedef struct {
    uint8_t         name[20];
    uint8_t         serialNumber[16];

    uint16_t        pId;
    uint8_t         channelCount;       
    uint8_t         swVersion;
    uint8_t         *buffer;        // Generic buffer for fetching device data
    cbus_dev_state_t state;
} comm_bus_device_t;

void CBUS_Init(void);
void CBUS_SVAR_Init(void);
uint8_t CBUS_sendData(uint32_t nodeAdd, uint8_t *data, uint8_t length);
uint8_t CBUS_getChannelData(uint8_t devId, uint8_t channelId, uint8_t *buffer, uint16_t bufferSize);
uint8_t CBUS_setChannelData(uint8_t devId, uint8_t channelId, uint8_t *buffer, uint16_t bufferSize);

#endif /* CBUS_APP_H_ */
