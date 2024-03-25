

#ifndef GSG_CORE_H_
#define GSG_CORE_H_

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

/*Standard Constants*/
#define PASS 	1
#define FAIL	0
#define TRUE 	1
#define FALSE	0
#define VALID	1
#define INVALID	0
#define ENABLE	1
#define DISABLE	0

#include "sys_config.h"
#include "string.h"

#ifdef	SYS_USE_MOD_QUEUE
#include "sys_queue.h"
#endif


#ifdef	SYS_USE_MOD_GPIO
#include "GPIO.h"
#endif

#define 	MCU_SERIES_AVR_8_BIT 	1
#define 	MCU_SERIES_STM32		2

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

#endif

/*

#ifndef GSG_XYZ_H_
#define GSG_XYZ_H_


#include <core.h>




#endif


 * */
