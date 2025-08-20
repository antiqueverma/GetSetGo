/*
 * NVM_Config.h
 *
 *  Created on: May 1, 2023
 *      Author: antiq
 */
#ifndef INC_NVM_CONFIG_H_
#define INC_NVM_CONFIG_H_

#define NVM_START_ADD				0x00
#define NVM_SIZE					1024		//Total Size in bytes of NVM in use
#define NVM_WORD_SIZE				2			//Size of smallest data storage unit
#define NVM_CELL_RST_VALUE			0xFFFF
#define NVM_USE_DUMMY_NVM

#define NVM_PAGE_DIVISION_ENABLE
#define NVM_PAGE_SIZE		100			//Size of a single page (in words)

/******************************************
 * NVM Page Base Addresses
 *****************************************/
#define NVM_ADD_NW_DATA_BASE						NVM_PAGE_SIZE*0
#define NVM_ADD_NODE_DATA_BASE						NVM_PAGE_SIZE*1

/******************************************
 * NVM Expanded Addresses
 *****************************************/
/*Network Details Page: SYS_DATA*/
	#define NVM_ADD_SYS_UNID				0
	#define	NVM_ADD_CURR_DYN_PSWD			1
	#define NVM_ADD_CURR_STAT_PSWD			2
	#define NVM_ADD_NODE_COUNT				3
	#define NVM_ADD_SYS_DATA(Property)		(NVM_ADD_NW_DATA_BASE+Property)

/*Node Details Page: NODE_DATA*/
	#define NVM_ADD_NODE_SERIAL_NUM			0
	#define NVM_ADD_NODE_INDEX				1
	#define NVM_ADD_NODE_STATE				2
	#define NVM_ADD_NODE_LAST_DYN_PSWD		3
	#define NVM_ADD_NODE_DATA_PARAM0		4
	//Max Size of Node Details can be 10 Words
	#define NVM_ADD_NODE_DATA(nodeIndex,Property)	((NVM_ADD_NODE_DATA_BASE*nodeIndex)+Property)

/*PAGE2 STARTS HERE*/


#endif /* INC_NVM_CONFIG_H_ */
