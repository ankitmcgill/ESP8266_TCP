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

#define ESP8266_TCP_DEBUG_ON
#define ESP8266_TCP_DNS_MAX_TRIES		5
#define ESP8266_TCP_GET_REQUEST_STRING "GET %s HTTP/1.1\r\nHost: %s\r\n\r\n"

//CUSTOM VARIABLE STRUCTURES/////////////////////////////
typedef enum
{
	ESP8266_TCP_STATE_DNS_RESOLVED,
	ESP8266_TCP_STATE_ERROR,
	ESP8266_TCP_STATE_OK
} ESP8266_TCP_STATE;

typedef struct
{
	char extracted_data_start_match_string[50];
	uint8_t extracted_data_offset_from_match_string;
	uint8_t extracted_data_char_len;
	char extracted_data_terminating_char;
	char extracted_data[50];
}ESP8266_TCP_EXTRACTED_DATA;

typedef struct
{
	char tcp_reply_packet_terminating_chars[10];
	uint8_t tcp_reply_extracted_data_count;
	ESP8266_TCP_EXTRACTED_DATA* tcp_reply_extracted_data;
}ESP8266_TCP_USER_DATA_CONTAINER;
//END CUSTOM VARIABLE STRUCTURES/////////////////////////

//FUNCTION PROTOTYPES/////////////////////////////////////
//CONFIGURATION FUNCTIONS
void ICACHE_FLASH_ATTR ESP8266_TCP_Initialize(const char* hostname,
													const char* host_ip,
													uint16_t host_port,
													const char* host_path,
													uint32_t tcp_connection_interval_ms);
void ICACHE_FLASH_ATTR ESP8266_TCP_Intialize_Request_Buffer(uint32_t buffer_size);
void ICACHE_FLASH_ATTR ESP8266_TCP_Initialize_UserDataContainer(ESP8266_TCP_USER_DATA_CONTAINER* container);
void ICACHE_FLASH_ATTR ESP8266_TCP_SetDnsServer(char num_dns, ip_addr_t* dns);
void ICACHE_FLASH_ATTR ESP8266_TCP_SetCallbackFunctions(void (*tcp_con_cb)(void*),
															void (*tcp_discon_cb)(void*),
															void (tcp_send_cb)(void*),
															void (tcp_recv_cb)(void*, char*, unsigned short),
															void (user_data_ready_cb)(ESP8266_TCP_EXTRACTED_DATA*, ESP8266_TCP_STATE));

//GET PARAMETERS FUNCTIONS
uint32_t ICACHE_FLASH_ATTR ESP8266_TCP_GetDataAcquisitionInterval(void);
const char* ICACHE_FLASH_ATTR ESP8266_TCP_GetSourceHost(void);
const char* ICACHE_FLASH_ATTR ESP8266_TCP_GetSourcePath(void);
uint16_t ICACHE_FLASH_ATTR ESP8266_TCP_GetSourcePort(void);
ESP8266_TCP_STATE ICACHE_FLASH_ATTR ESP8266_TCP_GetState(void);

//CONTROL FUNCTIONS
void ICACHE_FLASH_ATTR ESP8266_TCP_ResolveHostName(void (*user_dns_cb_fn)(ip_addr_t*));
void ICACHE_FLASH_ATTR ESP8266_TCP_StartDataAcqusition(void);
void ICACHE_FLASH_ATTR ESP8266_TCP_StopDataAcquisition(void);

//INTERNAL CALLBACK FUNCTIONS
void ICACHE_FLASH_ATTR _esp8266_tcp_dns_timer_cb(void* arg);
void ICACHE_FLASH_ATTR _esp8266_tcp_dns_found_cb(const char* name, ip_addr_t* ipAddr, void* arg);
void ICACHE_FLASH_ATTR _esp8266_tcp_connect_cb(void* arg);
void ICACHE_FLASH_ATTR _esp8266_tcp_disconnect_cb(void* arg);
void ICACHE_FLASH_ATTR _esp8266_tcp_send_cb(void* arg);
void ICACHE_FLASH_ATTR _esp8266_tcp_receive_cb(void* arg, char* pusrdata, unsigned short length);
void ICACHE_FLASH_ATTR _esp8266_tcp_data_acquisition_timer_cb(void* arg);
//END FUNCTION PROTOTYPES/////////////////////////////////
#endif
