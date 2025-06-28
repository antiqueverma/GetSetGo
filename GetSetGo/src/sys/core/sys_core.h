

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
	PERI_GPIOA,
	PERI_GPIOB,
	PERI_GPIOC,
	PERI_GPIOD,
	PERI_GPIOE,
	PERI_GPIOF,
	PERI_GPI0G,
	PERI_GPIOH,
	PERI_GPIOI,
	PERI_GPIOJ,
	PERI_GPIOK,
	PERI_GPIOL,

	PERI_TIMER0,
	PERI_TIMER1,
	PERI_TIMER2,
	PERI_TIMER3,
	PERI_TIMER4,
	PERI_TIMER5,

	PERI_UART0,
	PERI_UART1,
	PERI_UART2,
	PERI_UART3,
	PERI_UART4,
	PERI_UART5,

	PERI_SPI0,
	PERI_SPI1,
	PERI_SPI2,
	PERI_SPI3,
	PERI_SPI4,
	PERI_SPI5,

	PERI_I2C0,
	PERI_I2C1,
	PERI_I2C2,
	PERI_I2C3,
	PERI_I2C4,
	PERI_I2C5,

	PERI_CAN0,
	PERI_CAN1,
	PERI_CAN2,
	PERI_CAN3,
	PERI_CAN4,
	PERI_CAN5,

	PERI_ADC0,
	PERI_ADC1,
	PERI_ADC2,
	PERI_ADC3,
	PERI_ADC4,
	PERI_ADC5,

	PERI_DAC0,
	PERI_DAC1,
	PERI_DAC2,
	PERI_DAC3,
	PERI_DAC4,
	PERI_DAC5,

	PERI_COMP0,
	PERI_COMP1,
	PERI_COMP2,
	PERI_COMP3,
	PERI_COMP4,
	PERI_COMP5,

	PERI_DMA0,
	PERI_DMA1,
	PERI_DMA2,
	PERI_DMA3,
	PERI_DMA4,
	PERI_DMA5,

	PERI_EXT_INT,
	PERI_EXT_INTn = PERI_EXT_INT+100,

	PERI_RTC,
	PERI_WDT,

	PERI_RANDOM,
	PERI_CRYPTO,
	PERI_CRC,
	PERI_HASH,

	PERI_USB,
	PERI_WIFI,
	PERI_BLUETOOTH,
	PERI_BLUETOOTH_LE,
	PERI_ETHERNET,

	PERI_RGB24B,

	PERI_FLASH,
	PERI_SRAM,
	PERI_EEPROM,

	PERI_CAPTOUCH

} sys_peripheral;

#endif

