/*
 * EspCmd.c
 *
 *  Created on: Dec 24, 2024
 *      Author: antiq
 */

#include "ESP_AT.h"

char *CMD_OUT[__AT_T_LASTCMD] = {
		[AT_T] 					= "AT",                     // Handshaking Command
		[AT_T_VERSION] 			= "AT+GMR",         // ESP Version
		[AT_T_ECHO_OFF] 		= "ATE0",          // ECHO
		[AT_T_SET_BAUD_CUR] 	= "AT+UART_CUR=", // Set Current Baud rate
		[AT_T_CHK_CUR_MODE] 	= "AT+CWMODE_CUR?", // Check current mode (STA/AP)
		[AT_T_SET_CUR_MODE]		= "AT+CWMODE_CUR=", // Set current mode (STA/AP)
		[AT_T_LIST_AVLB_AP] 	= "AT+CWLAP",  // List all available APs
		[AT_T_CONNECT_2_AP_CUR] = "AT+CWJAP_CUR=", // Connect to AP (current)
		[AT_T_CONNECT_2_AP_DEF] = "AT+CWJAP_DEF=", // Connect to AP (default)
		[AT_T_CHK_CUR_AP] 		= "AT+CWJAP_CUR?", // Query Current AP
		[AT_T_DISCON_AP] 		= "AT+CWQAP",     // Disconnect from current AP
		[AT_T_SET_HOSTNAME]		= "AT+CWHOSTNAME?",	//Set station mode name for network scans
		[AT_T_CHK_CUR_IP] 		= "AT+CIPSTA_CUR?", // Query Current IP address
		[AT_T_START_CON] 		= "AT+CIPSTART=", // Start a new connection
		[AT_T_START_MDNS] 		= "AT+MDNS=",    // Start MDNS Server
		[AT_T_SET_HOST_NAME] 	= "AT+CWHOSTNAME=", // Set the host name
		[AT_T_CONFIG_TCP_SERVER]= "AT+CIPSERVER=", // Configure TCP Server
		[AT_T_TCP_CL_DCON_TO] 	= "AT+CIPSTO=", // Disconnect timeout for idle TCP Clients
		[AT_T_SETUP_CONNECTION] = "AT+CIPSTART=", // Establish a connection
		[AT_T_RESET] 			= "AT+RST",          // Reset
		[AT_T_SEND_DATA] 		= "AT+CIPSEND",   // Send data
		[AT_T_SET_MULTI_CONNECT]= "AT+CIPMUX=",		//Set Multi Connection Mode
		[AT_T_SET_ADD_PORT_RX_PKT]= "AT+CIPDINFO=",	//Set Multi Connection Mode
		[AT_T_SET_CIPMODE]		= "AT+CIPMODE",
		[AT_T_CIPCLOSE]			= "AT+CIPCLOSE"
};

char *CMD_IN[__AT_R_LASTCMD] = {
    [AT_R_OK]                = "OK",
    [AT_R_ERR]                = "ERROR",
    [AT_R_BUSY]               = "busy p...",
    [AT_R_CUR_MODE]           = "+CWMODE_CUR:",
    [AT_R_NO_AP]              = "No AP",
    [AT_R_WIFI_CONN]          = "WIFI CONNECTED",
    [AT_R_WIFI_DISCON]        = "WIFI DISCONNECT",
    [AT_R_STA_CUR_IP]         = "+CIPSTA_CUR:ip:",
    [AT_R_STA_CUR_GTW]        = "+CIPSTA_CUR:gateway:",
    [AT_R_STA_CUR_NET_MSK]    = "+CIPSTA_CUR:netmask:",
    [AT_R_CUR_AP]             = "+CWJAP_CUR:",
    [AT_R_GOT_IP]             = "WIFI GOT IP",
    [AT_R_RX_DATA]            = "+IPD,",
    [AT_R_NO_CHANGE]          = "no change",
    [AT_R_LINK_ALREADY_BUILT] = "link is builded",
    [AT_R_ALREADY_CONN]       = "ALREADY CONNECTED",
    [AT_R_CONNECT]            = "CONNECT",
    [AT_R_LINK_CONNECT_0]     = "0,CONNECT",
	[AT_R_LINK_CONNECT_1]     = "1,CONNECT",
	[AT_R_LINK_CONNECT_2]     = "2,CONNECT",
	[AT_R_LINK_CONNECT_3]     = "3,CONNECT",
    [AT_R_LINK_CLOSED]        = ",CLOSED",
	[AT_R_CLOSED]			  = "CLOSED",
    [AT_R_RDY_2_ACCP_TXD]     = ">",
    [AT_R_RCVD_BYTES]         = "Recv",
    [AT_R_SEND_OK]            = "SEND OK",
    [AT_R_BUSY_S]             = "busy s...",
    [AT_R_ATVERSION]		  = "AT version:"
};

