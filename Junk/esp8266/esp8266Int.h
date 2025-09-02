/*
 * ESP_AT_internal.h
 *
 *  Created on: Dec 24, 2024
 *      Author: antiq
 */

#ifndef ESP8266_INT_H_
#define ESP8266_INT_H_

#include "ESP8266.h"
typedef enum  {
	ESP_INIT,
	ESP_IDLE,
	ESP_RESET,
	ESP_START_TCP_SERVER,
	ESP_START_TCP_CLIENT,
	ESP_START_UDP_CLIENT,
	ESP_CONNECTION_ESTD,
	ESP_CONNECTED_IDLE,
	ESP_TXIP,
	ESP_OPEN_UDP_PROXY,
	ESP_UDP_PROXY,
	ESP_CLOSE_UDP_PROXY,
	ESP_RXIP
} esp_states;

typedef enum{
	ESP_TASK_TYPE_NONE,
	ESP_TASK_TYPE_RECONFIG_NETWORK,
	ESP_TASK_TYPE_DISCONNECT_NETWORK,
	ESP_TASK_TYPE_RECONNECT_NETWORK,

	// Socket Related
	ESP_TASK_TYPE_OPEN_SOCKET,
	ESP_TASK_TYPE_CLOSE_SOCKET,
	ESP_TASK_TYPE_RECONFIG_SOCKET,
} esp_task_type;

typedef struct{
	esp_task_type 	type;
	void *data;
	uint32_t		timeout;
} esp_task_t;

typedef struct{
	char 			targetIP[16];

	TaskHandle_t	task;
	uint16_t		port;
	uint8_t			linkId;
	uint8_t 		flags;

	uint16_t 		connTO;	// ms
	uint8_t 		dataFormat;		//0 for binary, 1 for ASCII

	uint16_t 		txQueueLength;
	uint16_t 		rxQueueLength;

	

	//uint16_t 		retryDelay;  //not needed
	//uint8_t 		retryCount;	//not needed
} esp_socket_nt;


typedef struct {
    TaskHandle_t 	task;        // Task requesting the connection

	char 			ipAddress[16];          // Target IP address
    uint16_t 		port;            // Target port for connection
    uint32_t 		timeStamp;       // Timeout for connection in milliseconds

    uint16_t		size;

    uint8_t 		priority;         // Priority of the connection request
    uint8_t 		flags;            // Optional flags for connection settings
    uint8_t			linkId;
    uint8_t		 	*buffer;  // Type of connection requested

} esp_dat_rqst_t;






typedef struct {
    TaskHandle_t 	task;        // Task requesting the connection

    uint16_t 		port;            // Target port for connection
    uint16_t 		timeoutMs;       // Timeout for connection in milliseconds

    uint8_t 		priority;         // Priority of the connection request
    uint8_t 		flags;            // Optional flags for connection settings
    uint8_t			linkId;

    char 			ipAddress[16];          // Target IP address

} esp_con_rqst_t;





typedef struct{
	char			ssid[20];
	char			password[20];
	char 			hostName[20];
	char 			espIP[15];
	char  			gtwIP[15];
	char 			subnetMask[15];

	uint8_t 		Conn_Count;
	esp_states 		state;
} esp_data_t;



#endif /* ESP8266_INT_H_ */
