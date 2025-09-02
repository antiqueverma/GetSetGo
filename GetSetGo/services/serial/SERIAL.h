
#ifndef SERIAL_H_
#define SERIAL_H_

#include "gsg_base.h"

typedef enum {
	SERIAL_PORT_IDLE = 0,
	SERIAL_PORT_SENDING,
	SERIAL_PORT_RECEIVING,
	SERIAL_PORT_ERROR
} serial_port_state_t;

typedef enum{
	SER_EVT_RX_DATA_READY	= (1<<0),
	SER_EVT_TX_COMPLETE		= (1<<1),
	SER_EVT_TERMINATOR_RCVD	= (1<<2)
} serial_event_t;

// A generic serial transmission function type
typedef void (*serial_tx_fn_t)(uint8_t *data, uint16_t length, uint16_t timeout);

typedef struct{
	QueueHandle_t 		rxQueue;
	QueueHandle_t 		txQueue;
	EventGroupHandle_t  eventGroup;
	TaskHandle_t 		taskhandle;		// optional for future use
	serial_tx_fn_t		tx_fn;
	serial_port_state_t state;
} serialPort_t;

gsg_result_t SER_openPort( serialPort_t *port, uint16_t txBuffSize, serial_tx_fn_t tx_fn, uint16_t rxBuffSize);
gsg_result_t SER_closePort(serialPort_t *port);
void SER_rxByteISRcb(serialPort_t *port, uint8_t byteReceived);
uint16_t SER_getRxData(serialPort_t *port, uint8_t *data, uint16_t length);
void SER_sendTxdata(serialPort_t *port, uint8_t *data, uint16_t length, uint16_t timeout);
void SER_flushRxBuffer(serialPort_t *port);
void SER_flushTxBuffer(serialPort_t *port);



#endif
