/*************************************************
* ESP8266 TCP LIBRARY
*
* MAY 11 2017
*
* ANKIT BHATNAGAR
* ANKIT.BHATNAGARINDIA@GMAIL.COM
* ***********************************************/

#ifndef _ESP8266_TCP_H_
#define _ESP8266_TCP_H_

#include "osapi.h"
#include "mem.h"
#include "ets_sys.h"
#include "ip_addr.h"
#include "espconn.h"
#include "os_type.h"

//CONFIGURATION FUNCTIONS
void ESP8266_TCP_Initialze();
void ESP8266_TCP_SetDataReadyCb();
void ESP8266_TCP_SetDnsServer1();
void ESP8266_TCP_SetDnsServer2();

//GET PARAMETERS FUNCTIONS
uint32_t ESP8266_TCP_GetDataAcquisitionInterval();
char* ESP8266_TCP_GetDataSourceHost();
char* ESP8266_TCP_GetDataSourcePath();
uint16_t ESP8266_TCP_GetDataSourcePort();

//CONTROL FUNCTIONS
void ESP8266_TCP_StartDataAcqusition();
void ESP8266_TCP_StopDataAcquisition();

#endif
