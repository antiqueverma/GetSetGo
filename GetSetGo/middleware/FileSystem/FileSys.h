/*
 * NVM.h
 *
 *  Created on: May 1, 2023
 *      Author: antiq
 */

#ifndef GSG_FILE_SYS_H_
#define GSG_FILE_SYS_H_

#include	"main.h"

typedef enum{
	//Data Structures
	FILE_TYPE_BYTE,			//unit data
	FILE_TYPE_BUFFER,		//arrays
	FILE_TYPE_LIST,			//structures
	FILE_TYPE_TABLE,		//2D arrays or 2x2-Lists
	//Text based files
	FILE_TYPE_TXT,
	FILE_TYPE_XML,
	FILE_TYPE_HTML,
	FILE_TYPE_CSS,
	FILE_TYPE_JSON,
	FILE_TYPE_TOML,
	//Configuration files, maybe another version of text files
	FILE_TYPE_CONFIG,	//Idk, maybe I just made this up?
	FILE_TYPE_INI,
	//Image files
	FILE_TYPE_BITMAP,
	FILE_TYPE_RGB24,
	FILE_TYPE_PNG,
	FILE_TYPE_JPEG,
	FILE_TYPE_JPG = FILE_TYPE_JPEG,
	//Audio files
	FILE_TYPE_WAV,
	FILE_TYPE_MP3,
	FILE_TYPE_M4A,
	//Programming file - A code that can be understood by any MCU!!!
	FILE_TYPE_PROGRAM,	//A revolutionary Idea! B-)

}file_types_t;

typedef struct{
	uint32_t add;
	uint32_t size;			//in bytes, or maybe KBs?
	uint32_t metainfo_1;	//Any specific information for a particular file type
	uint32_t metainfo_2;
	uint32_t metainfo_3;
	uint8_t type;
}file_handle_t;

typedef struct{
	uint32_t drive_size;		//in KBs
	uint32_t file_count;
	uint32_t start_address;		//in bytes
	uint32_t checksum;			//checksum/hash/hashmap/key/whatever related to security
	uint8_t  drive_type;		//Flash/EEPROM/RAM/SD/HDD
	uint8_t banks;				//partitions
}file_system_t;



#endif /* DRIVERS_NVM_H_ */
