#ifndef CAN_H_
#define CAN_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include <stdbool.h>

#include "gsg_base.h"

#include "gsg_mcu.h"

#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "gsg_base.h"

typedef enum
{
    CAN_ID_STANDARD = 0,
    CAN_ID_EXTENDED
} can_id_type_t;

typedef enum
{
    CAN_FRAME_DATA = 0,
    CAN_FRAME_REMOTE
} can_frame_type_t;

typedef struct
{
    uint32_t id;
    can_id_type_t idType;
    can_frame_type_t frameType;
    uint8_t dlc;
    uint8_t data[8];
} can_frame_t;

typedef struct can_port
{
    CAN_HandleTypeDef *canHandle;
    SemaphoreHandle_t mutex;
    SemaphoreHandle_t txCpltSema;
    QueueHandle_t txQueue;
    QueueHandle_t rxQueue;
    void (*rxCallback)(struct can_port *can, const can_frame_t *frame);
    void (*errorCallback)(struct can_port *can, uint32_t error);
} can_port_t;

gsg_result_t CAN_Init(can_port_t *can, CAN_HandleTypeDef *instance);
gsg_result_t CAN_DeInit(can_port_t *can);
gsg_result_t CAN_Start(can_port_t *can);
gsg_result_t CAN_Stop(can_port_t *can);
gsg_result_t CAN_SendFrame(can_port_t *can, const can_frame_t *frame, uint32_t timeout);
uint32_t CAN_GetError(can_port_t *can);

#ifdef __cplusplus
}
#endif

#endif /* CAN_H_ */
