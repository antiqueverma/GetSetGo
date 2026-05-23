/*
 * esp8266.h
 *
 *  Created on: Sep 2, 2025
 *      Author: antiq
 */



#ifndef ESP8266_H_
#define ESP8266_H_

#include "gsg_defs.h"
#include "services/serial/serial.h"
#include "services/debug/debug.h"

#define ESP_TASK_PRIORITY 	10
#define ESP_TASK_STACK_SIZE	KB_to_B(2)

#define ESP_SERIAL_RX_BUFF_SIZE KB_to_B(1)

#define ESP_CONFIG_NVM_OFFSET   100

typedef enum{
    SOCKET_TYPE_UDP_CLIENT = 0,
    SOCKET_TYPE_TCP_CLIENT,
    SOCKET_TYPE_TCP_SERVER,
} esp_socket_type_t;

typedef enum {
    SOCKET_STATE_CLOSED,
    SOCKET_STATE_CONNECTING,
    SOCKET_STATE_CONNECTED,
    SOCKET_STATE_LISTENING,
    SOCKET_STATE_SENDING,
    SOCKET_STATE_ERROR
} esp_socket_state_t;

typedef struct{
    uint8_t             socketId;
    esp_socket_type_t   type;
    uint16_t            localPort;
    uint16_t            remotePort;
    char                remoteIp[16];
    esp_socket_state_t  state;

    QueueHandle_t      txQueueHandle;
    QueueHandle_t      rxQueueHandle;

} esp_socket_t;


void ESP_Init(serial_port_t  *serialPort);


#endif /* ESP8266_H_ */

