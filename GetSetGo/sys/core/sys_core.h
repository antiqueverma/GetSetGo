

#ifndef GSG_CORE_H_
#define GSG_CORE_H_

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "string.h"

#include "sys_config.h"


/*Standard Constants*/
	#define PASS 	1
	#define FAIL	0
	#define TRUE 	1
	#define FALSE	0
	#define VALID	1
	#define INVALID	0
	//#define ENABLE	1
	//#define DISABLE	0

//Constant Macros
	#define BIT0 (1U<<0)	//0b1
	#define BIT1 (1U<<1)	//0b10
	#define BIT2 (1U<<2)	//0b100
	#define BIT3 (1U<<3)	//0b1000
	#define BIT4 (1U<<4)	//0b10000
	#define BIT5 (1U<<5)	//0b100000
	#define BIT6 (1U<<6)	//0b1000000
	#define BIT7 (1U<<7)	//0b10000000
	#define BIT8 (1U<<8)	//0b100000000
	#define BIT9 (1U<<9)	//0b1000000000
	#define BITA (1U<<10)	//0b10000000000
	#define BITB (1U<<11)	//0b100000000000
	#define BITC (1U<<12)	//0b1000000000000
	#define BITD (1U<<13)	//0b10000000000000
	#define BITE (1U<<14)	//0b100000000000000
	#define BITF (1U<<15)	//0b1000000000000000

//MCU Series
	#define 	MCU_SERIES_AVR_8_BIT 	1
	#define 	MCU_SERIES_STM32		2


#if(SYS_MCU_SERIES == MCU_SERIES_AVR_8_BIT)//AVR 8Bit
	#define MsDelay	_ms_delay
	#define UsDelay	_us_delay
	#define SYS_BUS_WIDTH	8
#elif(SYS_MCU_SERIES == MCU_SERIES_STM32)
	#define MsDelay	msDelay
	#define UsDelay	usDelay
	#define SYS_BUS_WIDTH	32
#elif(SYS_MCU_SERIES == MCU_SERIES_MSP430)
	#define MsDelay
	#define UsDelay
	#define SYS_BUS_WIDTH 16
#endif


/********************* Module Inclusions *********************/
#ifdef	SYS_USE_MOD_QUEUE
#include "sys_queue.h"
#endif

#ifdef	SYS_USE_MOD_GPIO
#include "drv_GPIO.h"
#endif

#ifdef 	SYS_USE_MOD_UART
#include "UART.h"
#endif

#ifdef 	SYS_USE_MOD_SERIAL
#include "SerialConfig.h"
#include "Serial.h"
#endif

#ifdef 	SYS_USE_MOD_LCD1602
#include "LCD1602.h"
#endif

#ifdef SYS_USE_MOD_NVM
#include "NVM_Config.h"
#include "NVM.h"
#endif

#ifdef SYS_USE_MOD_FAULT
#include "FAULT.h"
#endif
/*************************************************************/

typedef enum{
	MOD_GPIOA,
	MOD_GPIOB,
	MOD_GPIOC,
	MOD_GPIOD,
	MOD_GPIOE,
	MOD_GPIOF,
	MOD_GPI0G,
	MOD_GPIOH,
	MOD_GPIOI,
	MOD_GPIOJ,
	MOD_GPIOK,
	MOD_GPIOL,

	MOD_TIMER0,
	MOD_TIMER1,
	MOD_TIMER2,
	MOD_TIMER3,
	MOD_TIMER4,
	MOD_TIMER5,

	MOD_UART0,
	MOD_UART1,
	MOD_UART2,
	MOD_UART3,
	MOD_UART4,
	MOD_UART5,

	MOD_SPI0,
	MOD_SPI1,
	MOD_SPI2,
	MOD_SPI3,
	MOD_SPI4,
	MOD_SPI5,

	MOD_I2C0,
	MOD_I2C1,
	MOD_I2C2,
	MOD_I2C3,
	MOD_I2C4,
	MOD_I2C5,

	MOD_CAN0,
	MOD_CAN1,
	MOD_CAN2,
	MOD_CAN3,
	MOD_CAN4,
	MOD_CAN5,

	MOD_ADC0,
	MOD_ADC1,
	MOD_ADC2,
	MOD_ADC3,
	MOD_ADC4,
	MOD_ADC5,

	MOD_DAC0,
	MOD_DAC1,
	MOD_DAC2,
	MOD_DAC3,
	MOD_DAC4,
	MOD_DAC5,

	MOD_COMP0,
	MOD_COMP1,
	MOD_COMP2,
	MOD_COMP3,
	MOD_COMP4,
	MOD_COMP5,

	MOD_DMA0,
	MOD_DMA1,
	MOD_DMA2,
	MOD_DMA3,
	MOD_DMA4,
	MOD_DMA5,

	MOD_EXT_INT,
	MOD_EXT_INTn = MOD_EXT_INT+100,

	MOD_RTC,
	MOD_WDT,

	MOD_RANDOM,
	MOD_CRYPTO,
	MOD_CRC,
	MOD_HASH,

	MOD_USB,
	MOD_WIFI,
	MOD_BLUETOOTH,
	MOD_BLUETOOTH_LE,
	MOD_ETHERNET,

	MOD_RGB24B,

	MOD_FLASH,
	MOD_SRAM,
	MOD_EEPROM,

	MOD_CAPTOUCH

} sys_module;

#endif

