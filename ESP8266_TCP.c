/*************************************************
* ESP8266 TCP LIBRARY
*
* MAY 11 2017
*
* ANKIT BHATNAGAR
* ANKIT.BHATNAGARINDIA@GMAIL.COM
* ***********************************************/

#include "ESP8266_TCP.h"

//LOCAL LIBRARY VARIABLES/////////////////////////////////////
//TCP RELATED
static struct espconn _esp8266_tcp_espconn;
static struct _esp_tcp _esp8266_tcp_user_tcp;

//IP / HOSTNAME RELATED
static const char* _esp8266_tcp_host_name;
static ip_addr_t _esp8266_tcp_host_ip;
static ip_addr_t _esp8266_tcp_resolved_host_ip;
static const char* _esp8266_tcp_host_path;
static uint16_t _esp8266_tcp_host_port;

//TIMER RELATED
static uint32_t _esp8266_tcp_timer_interval;
static os_timer_t _esp8266_tcp_dns_timer;
static volatile os_timer_t _esp8266_tcp_timer;

//TCP OBJECT STATE
static char* _esp8266_tcp_get_request_buffer;
static enum ESP8266_TCP_STATE _esp8266_tcp_state;
//END LOCAL LIBRARY VARIABLES/////////////////////////////////

void ICACHE_FLASH_ATTR ESP8266_TCP_Initialize(char* hostname,
													ip_addr_t host_ip,
													uint16_t host_port,
													char* host_path,
													uint32_t tcp_connection_interval_ms)
{
	//INITIALIZE TCP CONNECTION PARAMETERS
	//HOSTNAME (RESOLVED THROUGH DNS IF HOST IP = NULL)
	//HOST IP
	//HOST PORT
	//HOST PATH
	//TCP CONNECTION INTERVAL MS

	_esp8266_tcp_host_name = hostname;
	_esp8266_tcp_host_ip = host_ip;
	_esp8266_tcp_host_port = host_port;
	_esp8266_tcp_host_path = host_path;
	_esp8266_tcp_timer_interval = tcp_connection_interval_ms;
}

void ICACHE_FLASH_ATTR ESP8266_TCP_Intialize_Request_Buffer(uint32_t buffer_size)
{
	//ALLOCATE THE ESP8266 TCP GET REQUEST BUFFER

	_esp8266_tcp_get_request_buffer = (char*)os_zalloc(buffer_size);

	//GENERATE THE GET STRING USING HOST-NAME & HOST-PATH
	os_sprintf(_esp8266_tcp_get_request_buffer, ESP8266_TCP_GET_REQUEST_STRING,
			_esp8266_tcp_host_name, _esp8266_tcp_host_path);
}

void ICACHE_FLASH_ATTR ESP8266_TCP_SetDnsServer(char num_dns, ip_addr_t* dns)
{
	//SET DNS SERVER RESOLVE HOSTNAME TO IP ADDRESS
	//MAX OF 2 DNS SERVER SUPPORTED (num_dns)

	if(num_dns == 1 || num_dns == 2)
	{
		espconn_dns_setserver(num_dns, dns);
	}
	return;
}

void ICACHE_FLASH_ATTR ESP8266_TCP_SetCallbackFunctions()
{
	//HOOK FOR THE USER TO PROVIDE CALLBACK FUNCTIONS FOR
	//VARIOUS INTERNAL TCP OPERATION
}

uint32_t ICACHE_FLASH_ATTR ESP8266_TCP_GetDataAcquisitionInterval(void)
{
	//RETURN ESP8266 TCP TIMER DATA ACQUISITION INTERVAL (MS)

	return _esp8266_tcp_timer_interval;
}

const char* ICACHE_FLASH_ATTR ESP8266_TCP_GetSourceHost(void)
{
	//RETURN HOST NAME STRING

	return _esp8266_tcp_host_name;
}

const char* ICACHE_FLASH_ATTR ESP8266_TCP_GetSourcePath(void)
{
	//RETURN HOST PATH STRING

	return _esp8266_tcp_host_path;
}

void ICACHE_FLASH_ATTR  ESP8266_TCP_ResolveHostName(void* user_dns_cb_fn)
{
	//RESOLVE PROVIDED HOSTNAME USING THE SUPPLIED DNS SERVER
	//AND CALL THE USER PROVIDED DNS DONE CB FUNCTION WHEN DONE

	//SET USER DNS RESOLVE CB FUNCTION

	//START THE DNS REOLVING PROCESS AND TIMER
	struct espconn pespconn;
	espconn_gethostbyname(&pespconn, _esp8266_tcp_host_name, &_esp8266_tcp_resolved_host_ip, _esp8266_tcp_dns_found_cb);
	os_timer_arm(&_esp8266_tcp_dns_timer, 1000, 0);
}

uint16_t ICACHE_FLASH_ATTR ESP8266_TCP_GetSourcePort(void)
{
	//RETURN HOST REMOTE PORT NUMBER

	return _esp8266_tcp_host_port;
}

void ICACHE_FLASH_ATTR ESP8266_TCP_StartDataAcqusition(void)
{
	//START TCP DATA ACUISITION CYCLE
}

void ICACHE_FLASH_ATTR ESP8266_TCP_StopDataAcquisition(void)
{
	//STOP TCP DATA ACUISITION CYCLE
}

void _esp8266_tcp_dns_timer_cb(void* arg)
{
	//ESP8266 DNS CHECK TIMER CALLBACK FUNCTIONS
	//TIME PERIOD = 1 SEC

	//DNS TIMER CB CALLED IE. DNS RESOLUTION DID NOT WORK
	//DO ANOTHER DNS CALL AND REARM THE TIMER
	struct espconn *pespconn = arg;
	espconn_gethostbyname(pespconn, _esp8266_tcp_host_name, &_esp8266_tcp_resolved_host_ip, _esp8266_tcp_dns_found_cb);
	os_timer_arm(&_esp8266_tcp_dns_timer, 1000, 0);
}

void _esp8266_tcp_dns_found_cb(const char* name, ip_addr_t* ipAddr, void* arg)
{
	//ESP8266 TCP DNS RESOLVING DONE CALLBACK FUNCTION

	//DISABLE THE DNS TIMER
	os_timer_disarm(&_esp8266_tcp_dns_timer);

	if(ipAddr == NULL)
	{
		//HOST NAME COULD NOT BE RESOLVED
		os_printf("hostname : %s, could not be resolved\n", _esp8266_tcp_host_name);

		//CALL USER PROVIDED DNS CB FUNCTION
		return;
	}

	//DNS GOT IP
	os_printf("hostname : %s, resolved. IP = %d.%d.%d.%d\n", *((uint8_t*)_esp8266_tcp_resolved_host_ip.addr),
																*((uint8_t*)_esp8266_tcp_resolved_host_ip.addr + 1),
																*((uint8_t*)_esp8266_tcp_resolved_host_ip.addr + 2),
																*((uint8_t*)_esp8266_tcp_resolved_host_ip.addr + 3));

	//CALL USER PROVIDED DNS CB FUNCTION
}
