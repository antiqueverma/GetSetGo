
#ifndef SERIAL_H_
#define SERIAL_H_

#include "gsg_defs.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "event_groups.h"

#define SER_TERMINATOR_NULL	 	'\0'
#define SER_TERMINATOR_CR	 	'\r'
#define SER_TERMINATOR_LF	 	'\n'

typedef enum {
	SERIAL_PORT_INVALID = 0,
	SERIAL_PORT_IDLE,
	SERIAL_PORT_SENDING,
	SERIAL_PORT_RECEIVING,
	SERIAL_PORT_ERROR
} serial_port_state_t ;

typedef enum {
	SERIAL_PORT_UART = 0,
	SERIAL_PORT_SPI,
	SERIAL_PORT_I2C,
	SERIAL_PORT_CAN,
	SERIAL_PORT_TCP,
	SERIAL_PORT_RS485,
	SERIAL_PORT_UDP,
	SERIAL_PORT_WEBSOCKET,

	__SERIAL_PORT_TYPE_MAX
} serial_port_type_t ;

typedef enum {
	SERIAL_USE_STATIC_TX_BUFFER = (1<<0),
	SERIAL_USE_STATIC_RX_BUFFER = (1<<1),
} serial_flags_t;

typedef enum {
	SER_EVT_RX_DATA_READY	= (1<<0),
	SER_EVT_TX_COMPLETE		= (1<<1),
	SER_EVT_TERMINATOR_RCVD	= (1<<2)
} serial_event_t;

// A generic serial transmission function type
typedef gsg_result_t (*serial_tx_fn_t)(void *ctx, uint8_t *data, uint16_t length, uint16_t timeout);
typedef gsg_result_t (*serial_rx_fn_t)(void *ctx, uint8_t *data, uint16_t length, uint16_t timeout);

typedef struct {
	uint8_t 			*rxBuffer;
	uint8_t 			*txBuffer;
	void   				*context;

	serial_tx_fn_t		sendData;
	serial_rx_fn_t 		receiveData;

	SemaphoreHandle_t 	ownershipMutex;
	TaskHandle_t 		ownerTask;

	SemaphoreHandle_t 	rxBufferMutex;
	SemaphoreHandle_t 	txBufferMutex;

	SemaphoreHandle_t 	rxDataCountSema;

	serial_port_type_t 	type;
	serial_port_state_t state;
	
	uint16_t 			txHead;
	uint16_t 			txTail;

	uint16_t 			txCount;
	uint16_t 			txBuffSize;

	uint16_t 			rxHead;
	uint16_t 			rxTail;

	uint16_t 			rxCount;
	uint16_t 			rxBuffSize;
	
	uint8_t 			flags;

	uint8_t 			txEnPin;
	uint8_t 			rxEnPin;
} serial_port_t;



#include "serial.h"

gsg_result_t SER_openPort( serial_port_t *port, serial_port_type_t type, uint16_t txBuffSize, uint16_t rxBuffSize, uint8_t flags, uint8_t transmitEnablePin, uint8_t receiveEnablePin, void *peripheralHandle);
gsg_result_t SER_registerHandlers(serial_port_t *port, serial_tx_fn_t txHandler, serial_rx_fn_t rxHandler);
gsg_result_t SER_closePort(serial_port_t *port);
gsg_result_t SER_acquirePort(serial_port_t *port, uint32_t timeout);
gsg_result_t SER_releasePort(serial_port_t *port);
gsg_result_t SER_sendData(serial_port_t *port, uint8_t *data, uint16_t length, uint16_t timeout);
gsg_result_t SER_receiveData(serial_port_t *port, uint8_t *data, uint16_t requestedLength, uint16_t *receivedLength, uint32_t timeout);
void SER_rxByteIsrCallback(void *ctx, uint8_t byte);


#endif
