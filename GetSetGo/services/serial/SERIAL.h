
#ifndef SERIAL_H_
#define SERIAL_H_

#include "gsg_defs.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "event_groups.h"
#include "services/debug/debug.h"

#define SER_TERMINATOR_NULL	 	'\0'
#define SER_TERMINATOR_CR	 	'\r'
#define SER_TERMINATOR_LF	 	'\n'

typedef enum {
	SERIAL_PORT_IDLE = 0,
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
	SERIAL_PORT_UDP,
	SERIAL_PORT_WEBSOCKET
} serial_port_type_t ;

typedef enum {
	SER_EVT_RX_DATA_READY	= (1<<0),
	SER_EVT_TX_COMPLETE		= (1<<1),
	SER_EVT_TERMINATOR_RCVD	= (1<<2)
} serial_event_t;

// A generic serial transmission function type
typedef void (*serial_tx_fn_t)(uint8_t *data, uint16_t length, uint16_t timeout);

typedef struct {
	QueueHandle_t 		rxQueue;
	QueueHandle_t 		txQueue;
	EventGroupHandle_t  eventGroup;		// Owner task can wait on multiple events
	TaskHandle_t 		taskhandle;		// optional for future use
	serial_tx_fn_t		tx_fn;
	serial_port_state_t state;
	serial_port_type_t type;
} serial_port_t;

gsg_result_t SER_openPort( serial_port_t *port, serial_port_type_t type, uint16_t txBuffSize, serial_tx_fn_t tx_fn, uint16_t rxBuffSize);
gsg_result_t SER_closePort(serial_port_t *port);
void SER_rxByteISRcb(serial_port_t *port, uint8_t byteReceived);
uint16_t SER_receiveData(serial_port_t *port, uint8_t *data, uint16_t length, uint16_t timeout);
void SER_sendData(serial_port_t *port, uint8_t *data, uint16_t length, uint16_t timeout);
void SER_sendString(serial_port_t *port, char *data, uint16_t timeout);
void SER_flushRxBuffer(serial_port_t *port);
void SER_flushTxBuffer(serial_port_t *port);

#endif
