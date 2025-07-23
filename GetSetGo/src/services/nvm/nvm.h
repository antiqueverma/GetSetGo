/*
 * NVM.h
 *
 *  Created on: May 1, 2023
 *      Author: antiq
 */

#ifndef DRIVERS_NVM_H_
#define DRIVERS_NVM_H_

#include	"main.h"

#ifndef NVM_SIZE
#warning "NVM_SIZE not defined. Default NVM size will be 1KB"
#define NVM_SIZE			1024
#endif

#ifndef NVM_WORD_SIZE
#warning "NVM_WORD_SIZE not defined. Default word size will be 1 Byte"
#define NVM_WORD_SIZE		1
#endif

#ifndef NVM_CELL_RST_VALUE
	#if	NVM_WORD_SIZE == 2
		#warning "NVM_CELL_RST_VALUE not defined. Default value will be 0xFFFF"
		#define NVM_CELL_RST_VALUE		0xFFFF
	#else
		#warning "NVM_CELL_RST_VALUE not defined. Default value will be 0xFF"
		#define NVM_CELL_RST_VALUE		0xFF
	#endif
#endif

#if	NVM_WORD_SIZE == 1
	#define NVM_Word_Type	unsigned char
#elif NVM_WORD_SIZE == 2
	#define NVM_Word_Type	unsigned int
#endif

#define NVM_Max_Valid_Address	(NVM_SIZE/NVM_WORD_SIZE)

unsigned char NVM_Write_Cell(unsigned int Address, NVM_Word_Type Byte);
NVM_Word_Type NVM_Read_Cell(unsigned int Address);
unsigned char NVM_Write_String(unsigned int Base_Address, NVM_Word_Type *String, unsigned char Length);


#endif /* DRIVERS_NVM_H_ */
