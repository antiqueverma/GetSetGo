/*
 * NVM.c
 *
 *  Created on: May 1, 2023
 *      Author: antiq
 */

#include "NVM.h"

#ifdef NVM_USE_DUMMY_NVM
		static NVM_Word_Type NVM_DUMMY_MEMORY[NVM_SIZE];
#endif

unsigned char NVM_Write_Cell(unsigned int Address, NVM_Word_Type Byte)
{
	if(Address >= NVM_Max_Valid_Address)
			return FAIL;

	#ifdef NVM_USE_DUMMY_NVM
		NVM_DUMMY_MEMORY[Address] = Byte;
	#endif

	return PASS;
}

NVM_Word_Type NVM_Read_Cell(unsigned int Address)
{
	unsigned int data = NVM_CELL_RST_VALUE;
	if(Address >= NVM_Max_Valid_Address)
			return data;

	#ifdef NVM_USE_DUMMY_NVM
		data = NVM_DUMMY_MEMORY[Address];
	#endif

	return data;
}

unsigned char NVM_Write_String(unsigned int Base_Address, NVM_Word_Type *String, unsigned char Length)
{
	unsigned int i=0;
	if(Base_Address >= NVM_Max_Valid_Address)
		return FAIL;

	for(i=0;i<Length;i++)
	{
		#ifdef NVM_USE_DUMMY_NVM
			NVM_DUMMY_MEMORY[Base_Address+i] = *(String+i);
		#endif
	}
	return PASS;
}
