

#ifndef GSG_CONFIG_H_
#define GSG_CONFIG_H_

#define SYS_MCU_SERIES	2

#define SYS_USE_MOD_GPIO
#define SYS_USE_MOD_QUEUE
#define SYS_USE_MOD_FAULT
#define SYS_USE_MOD_UART
#define SYS_USE_MOD_SERIAL
#define SYS_USE_MOD_LCD1602
#define SYS_USE_MOD_NVM
#define SYS_USE_MOD_FILE_SYSTEM

//TODO: Unimplemented
//User nneds to define a buffer size be using below constant, otherwise common buffer size will be used
#define UART1_RX_BUFF_SIZE			100
//#define UART1_TX_BUFF_SIZE			100



#include "sys_core.h"



#endif
