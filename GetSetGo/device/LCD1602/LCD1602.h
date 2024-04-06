/********************************************************************
 * LCD1602.h													   		*
 * A platform independent Embedded-C driver for LCD1602        		*
 * The driver has been tested on the below platforms				*
 * 		1. Atmega328p (using AtmelStudio7)							*
 * 		2. MSP430FR2355 (using Code Composer Studio)				*
 * 		3. STM32F407VET6	(using STMCube IDE)						*
 * 																	*
 * Source datasheet link:											*
 * https://www.sparkfun.com/datasheets/LCD/HD44780.pdf         		*
 *																	*
 * 	Program Memory Usage 	:	1.8 KB approx                          *
 *	Data Memory Usage 		:	12 bytes                            *
 *  Author	: Manish Verma                                      	*
 *  E-mail	: manishv3898@gmail.com , antiQueverma@gmail.com		*
 *  LinkedIn:https://www.linkedin.com/in/manish-verma-551238154		*
 ********************************************************************/

#ifndef LCD1602_H_
#define LCD1602_H_
#include "sys_core.h"


#define DDRA 	(GPIOA->MODER)
#define OUTA	(GPIOA->ODR)
#define INA		(GPIOA->IDR)
#define OTYPA	(GPIOA->OTYPER)
#define PUPDRA	(GPIOA->PUPDR)


#define DDRB 	(GPIOB->MODER)
#define OUTB	(GPIOB->ODR)
#define INB		(GPIOB->IDR)
#define OTYPB	(GPIOB->OTYPER)
#define PUPDRB	(GPIOB->PUPDR)

#define DDRC 	(GPIOC->MODER)
#define OUTC	(GPIOC->ODR)
#define INC		(GPIOC->IDR)
#define OTYPC	(GPIOC->OTYPER)
#define PUPDRC	(GPIOC->PUPDR)

#define DDRD 	(GPIOD->MODER)
#define OUTD	(GPIOD->ODR)
#define IND		(GPIOD->IDR)
#define OTYPD	(GPIOD->OTYPER)
#define PUPDRD	(GPIOD->PUPDR)

//Macros for LCD connections with the MCU	Adding the below code is mandatory
#define LCD_DPORT		     OUTD					//Output register for LCD D0-D7 pin connections
#define LCD_CPORT		     OUTA					//Output register for LCD RS, RW, EN pin connections
#define LCD_DDIR		     DDRD				//Direction register for LCD D0-D7 data pins
#define LCD_CDIR		     DDRA				//Direction register for LCD RS, RW, EN pins
#define LCD_DINP		     IND					//Data input register for LCD D0-D7 pins
#define LCD_RSPIN		     BIT2						//Bit position for RS pin
#define LCD_RWPIN			 BIT1						//Bit position for RW pin
#define LCD_ENPIN			 BIT0						//Bit position for EN pin
#define LCD_DELAYMS		     MsDelay					//Platform specific millisecond delay function
#define LCD_DELAYUS		     UsDelay					//Platform specific microsecond delay function

//Macros to be used as arguments to the initialization function
#define FourBit			0
#define EightBit		1
#define UpperNibble 	1
#define LowerNibble 	0
#define Stm32Based		1
#define NonStm32Based	0

//Binary Constants
/*#define BIT0 0b1
#define BIT1 0b10
#define BIT2 0b100
#define BIT3 0b1000
#define BIT4 0b10000
#define BIT5 0b100000
#define BIT6 0b1000000
#define BIT7 0b10000000*/
//LCD operation command Macros
#define LCDCLEAR		0b1//BIT0
#define RETURNHOME		0b10//BIT1
#define INCCR			0b110//(BIT2|BIT1)
#define DECCR			0b100//(BIT2)
#define LSHIFTTEXT		0b11000 //(BIT4| BIT3)
#define RSHIFTTEXT		0b11100//(BIT4 | BIT3 | BIT2)
#define DISPON			0b1100//(BIT3|BIT2)
#define DISPOFF			0b1000//(BIT3)
#define CDISPON			0b1110//(BIT3|BIT2|BIT1)
#define CDISPOFF		0b1100//(BIT3|BIT2)
#define CBLNKON			0b1111//(BIT3|BIT2|BIT1|BIT0)
#define CBLNKOFF		0b1110//(BIT3|BIT2|BIT1)
#define BIT4MODE		0b0000101000
#define BIT8MODE		0b0000111000
#define BIT8MODEINIT	0b110000
#define BIT4MODEINIT	0b0011
#define FWMODE			0b111000		//1602 as default mode
#define HWMODE			0b101000		//1602 as default mode
#define BSYFLG			0b10000000//BIT7
#define CPOS			0b10000000//BIT7
#define LCD_STM32OUT	0b01	//Setting GPIOx->MODER register bit pairs into output direction
#define LCD_STM32IN		0b00	//Setting GPIOx->MODER register bit pairs into output direction
//end

//LCD configuration register bits
#define LCD_BM			0b1//BIT0
#define LCD_NP			0b10//BIT1
#define LCD_CL			0b111100000000
#define LCD_RW			0b1111000000000000
#define LCD_STMDEV		0b100

//Mode Macros
#define LCD_WRITE	(LCD_CPORT &= ~LCD_RWPIN)
#define LCD_READ	(LCD_CPORT |= LCD_RWPIN)
#define LCD_ENABLE	(LCD_CPORT |= LCD_ENPIN )
#define LCD_DISABLE (LCD_CPORT &= ~LCD_ENPIN )
#define LCD_IMODE	(LCD_CPORT &= ~LCD_RSPIN)
#define LCD_DMODE	(LCD_CPORT |= LCD_RSPIN)
#define LCD_POFF	4
#endif

/************************************************************************************
 * Function Prototypes begin here
 * All functions with their names in upper-case serve the purpose of API for the LCD
 * All functions with their names in lower-case serve the purpose internal functions
 */
//User Interface functions
void LCD_INIT(int LCD_stm_dev, int LCD_BITMODE, int LCD_NIBPOS, int LCD_ROW, int LCD_COLUMN);
void LCD_DATA(char x);
void LCD_STRING(char[]);
void LCD_INTEGER(int num);
void LCD_BINARY(int num, int hzero);
void LCD_FLOAT(float num);
void LCD_CLEAR();
void LCD_SHIFTR(void);
void LCD_SHIFTL(void);
void LCD_CUROFF();
void LCD_HOME();
void LCD_SETCUR(char row, char col);
void LCD_NEWMSG(char lcd_string[]);
void LCD_NEWMSG_L2(char lcd_string[]);
//Internal functions not intended to be used by the user
void LCD_8bit_bus(char lcd_cmd);
void LCD_command (char lcd_cmd);
int  LCD_8bit_busy(void);
void LCD_4bit_bus (char bus_val);
int LCD_4bit_busy(void);
int LCD_bpshifter(int lcd_input, int lcd_pairpatt);		//Bit pattern shifting function for STM32 devices
void LCD_epulse();
void LCD_datMode();
void LCD_cmdRead();
void LCD_cmdWrite();
