/*
 * SerialConfig.h
 *
 *  Created on: Feb 1, 2022
 *      Author: antiq
 */

#ifndef SERIALCONFIG_H_
#define SERIALCONFIG_H_
	#include "main.h"
	/************************************************
	 * Use these values to define the method for Serial Drivers:
	 * 		1 -> FreeRTOS
	 * 		2 -> Procedural
	 ***********************************************/
	#define SER_IMP										2

    #define SER_TX_BUFF_SIZE 							8		//Size in bytes
    #define SER_RX_BUFF_SIZE 							100		//Size in bytes. This size will be used for queue implementation
	#define SER_RX_DATA_BUFF_SIZE 						100		//This will be the Actual size of the Rx Buffer
	#define SER_USE_RX_ISR								//Un-comment to use ISR for reception
	#define SER_RX_IRQ_HANDLER							USART1_IRQHandler	//Enter the Reception Interrupt Handler name used for the MCU
	#define SER_RX_TERMINATOR_COUNT						2			//Define the number of terminator chars to end of a message
	#define SER_RX_TERMINATOR_STR						"\r\n"
	#define SER_DREG									UART_DREG
	#define SER_RX_TASK_PRIORITY						4
	#define SER_TX_TASK_PRIORITY						2
	#define SER_INBOX_QUERY_PERIOD						100		//Enter time in milliseconds
	#define SER_OUTBOX_QUERY_PERIOD						500		//Enter time in milliseconds
	#define SER_RX_PROCESSING_LEVEL						0		//Enter 0 for message level or 1 for byte level. Byte level is only applicable for HMI type Serial Devices
	#define SER_RX_QUEUE_SIZE							10		//Reception Queue Depth - Applicable only for RTOS
	#define SER_TX_QUEUE_SIZE							3		//Applicable only for RTOS
	#define SER_TX_USE_TERMINATORS		//Define this as 1 to use terminators in transmission
	#define SER_TX_TERM_STR			"\r\n"  //Define a string to be used as terminator

#endif /* SERIALCONFIG_H_ */
