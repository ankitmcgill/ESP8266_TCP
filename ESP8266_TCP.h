/****************************************************************
* ESP8266 TCP LIBRARY
*
* MAY 11 2017
*
* ANKIT BHATNAGAR
* ANKIT.BHATNAGARINDIA@GMAIL.COM
*
* REFERENCES
* ------------
* 	(1) https://espressif.com/en/support/explore/sample-codes
* 	(2)
****************************************************************/

#ifndef _ESP8266_TCP_H_
#define _ESP8266_TCP_H_

#include "osapi.h"
#include "mem.h"
#include "ets_sys.h"
#include "ip_addr.h"
#include "espconn.h"
#include "os_type.h"

#define ESP8266_TCP_GET_REQUEST_STRING "GET /%s HTTP/1.1\r\nHost: %s\r\n\r\n"

//CUSTOM VARIABLE STRUCTURES/////////////////////////////
enum
{
	ESP8266_TCP_STATE_DNS_RESOLVED,
	ESP8266_TCP_STATE_REQUEST_SENT,
	ESP8266_TCP_STATE_REPLY_RECEIVED,
	ESP8266_TCP_STATE_SLEEPING,
	ESP8266_TCP_STATE_ERROR
} ESP8266_TCP_STATE;
//END CUSTOM VARIABLE STRUCTURES/////////////////////////

//FUNCTION PROTOTYPES/////////////////////////////////////
//CONFIGURATION FUNCTIONS
void ICACHE_FLASH_ATTR ESP8266_TCP_Initialize(char* hostname,
													ip_addr_t host_ip,
													uint16_t host_port,
													char* host_path,
													uint32_t tcp_connection_interval_ms);
void ICACHE_FLASH_ATTR ESP8266_TCP_Intialize_Request_Buffer(uint32_t buffer_size);
void ICACHE_FLASH_ATTR ESP8266_TCP_SetDnsServer(char num_dns, ip_addr_t* dns);
void ICACHE_FLASH_ATTR ESP8266_TCP_SetCallbackFunctions();

//GET PARAMETERS FUNCTIONS
uint32_t ICACHE_FLASH_ATTR ESP8266_TCP_GetDataAcquisitionInterval(void);
const char* ICACHE_FLASH_ATTR ESP8266_TCP_GetSourceHost(void);
const char* ICACHE_FLASH_ATTR ESP8266_TCP_GetSourcePath(void);
uint16_t ICACHE_FLASH_ATTR ESP8266_TCP_GetSourcePort(void);

//CONTROL FUNCTIONS
void ICACHE_FLASH_ATTR ESP8266_TCP_ResolveHostName(void);
void ICACHE_FLASH_ATTR ESP8266_TCP_StartDataAcqusition(void);
void ICACHE_FLASH_ATTR ESP8266_TCP_StopDataAcquisition(void);

//INTERNAL CALLBACK FUNCTIONS
void _esp8266_tcp_dns_timer_cb(void* arg);
void _esp8266_tcp_dns_found_cb(const char* name, ip_addr_t* ipAddr, void* arg);
void _esp8266_tcp_connect_cb();
void _esp8266_tcp_disconnect_cb();
void _esp8266_tcp_send_cb();
void _esp8266_tcp_receive_cb();

//END FUNCTION PROTOTYPES/////////////////////////////////
#endif
