/*
 * UART.h
 *
 * Any terminators to be appended is not supported by this library. It should be done by higher layers.
 * Please update system specific terminators in the ISR
 * A trash version is inside the ProtoHoAuHub project folder
 * Created: 17-03-2021 16:18:49
 *  Author: antiq
 */


#ifndef UART_H_
#define UART_H_

#include "sys_config.h"
#define MAX_MODS	6
#define UART_DREG (USART1->DR)
//Macros to be used as arguments to functions
#define UART_NO_PARITY    	0
#define UART_EVEN_PARITY  	1
#define UART_ODD_PARITY  	2


#define SUPP_BAUD_RATES		6	//Number of supported baud rates

typedef struct{
	uint32_t	speed;
	uint32_t 	config;
	uint8_t 	cbInstance;

	uint8_t		txBusy:4;
	uint8_t		rxBusy:4;

	bool		(*txbyte)(uint8_t,uint8_t);
	bool		(*rxbyte)(uint8_t,uint8_t*);

}scomObject_t;

void UART_INIT(scomObject_t *objUART,uint8_t instance,uint32_t baud_rate,uint8_t parity,uint8_t rxen);     //Use 8 bits, even parity and 2 stop bits

void UART_TxSTRING (uint8_t instance, uint8_t string[]);
void UART_TxBUFFER(uint8_t instance,uint8_t tx_str[], uint16_t length);
bool UART_TxBYTE(uint8_t instance, uint8_t tx_data);
//long UART_querBAUD();
//unsigned char UART_chkBAUD(long int baud_rate);
bool UART_RxBYTE(uint8_t instance, uint8_t *rx_data);
void USART1_IRQHandler (void);
#endif /* UART_H_ */

//Baud Rates

