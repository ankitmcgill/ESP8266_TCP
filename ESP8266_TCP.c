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
static const char* _esp8266_tcp_host_ip;
static ip_addr_t _esp8266_tcp_resolved_host_ip;
static const char* _esp8266_tcp_host_path;
static uint16_t _esp8266_tcp_host_port;

//TIMER RELATED
static volatile os_timer_t _esp8266_tcp_dns_timer;
static volatile os_timer_t _esp8266_tcp_timer;
static uint32_t _esp8266_tcp_timer_interval;
static uint16_t _esp8266_dns_retry_count;

//TCP OBJECT STATE
static char* _esp8266_tcp_get_request_buffer;
static enum ESP8266_TCP_STATE _esp8266_tcp_state;

//CALLBACK FUNCTION VARIABLES
static void (*dns_cb_function)(ip_addr_t*);
//END LOCAL LIBRARY VARIABLES/////////////////////////////////

void ICACHE_FLASH_ATTR ESP8266_TCP_Initialize(const char* hostname,
													const char* host_ip,
													uint16_t host_port,
													const char* host_path,
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
			_esp8266_tcp_host_path, _esp8266_tcp_host_name);

	os_printf("GET STRING : %s\n", _esp8266_tcp_get_request_buffer);
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

void ICACHE_FLASH_ATTR  ESP8266_TCP_ResolveHostName(void (*user_dns_cb_fn)(ip_addr_t*))
{
	//RESOLVE PROVIDED HOSTNAME USING THE SUPPLIED DNS SERVER
	//AND CALL THE USER PROVIDED DNS DONE CB FUNCTION WHEN DONE

	//DONE ONLY IF THE HOSTNAME SUPPLIED IN INITIALIZATION FUNCTION
	//IS NOT NULL. IF NULL, USER SUPPLIED IP ADDRESS IS USED INSTEAD
	//AND NO DNS REOSLUTION IS DONE

	//SET USER DNS RESOLVE CB FUNCTION
	dns_cb_function = user_dns_cb_fn;

	//SET DNS RETRY COUNTER TO ZERO
	_esp8266_dns_retry_count = 0;

	if(_esp8266_tcp_host_name != NULL)
	{
		//NEED TO DO DNS RESOLUTION

		//START THE DNS RESOLVING PROCESS AND TIMER
		_esp8266_tcp_resolved_host_ip.addr = 0;
		espconn_gethostbyname(&_esp8266_tcp_espconn, _esp8266_tcp_host_name, &_esp8266_tcp_resolved_host_ip, _esp8266_tcp_dns_found_cb);
		os_timer_setfn(&_esp8266_tcp_dns_timer, (os_timer_func_t*)_esp8266_tcp_dns_timer_cb, &_esp8266_tcp_espconn);
		os_timer_arm(&_esp8266_tcp_dns_timer, 1000, 0);
		return;
	}

	//NO NEED TO DO DNS RESOLUTION. USE USER SUPPLIED IP ADDRESS STRING
	_esp8266_tcp_resolved_host_ip.addr = ipaddr_addr(_esp8266_tcp_host_ip);

	//CALL USER SUPPLIED DNS RESOLVE CB FUNCTION
	(*dns_cb_function)(&_esp8266_tcp_resolved_host_ip);
}

uint16_t ICACHE_FLASH_ATTR ESP8266_TCP_GetSourcePort(void)
{
	//RETURN HOST REMOTE PORT NUMBER

	return _esp8266_tcp_host_port;
}

void ICACHE_FLASH_ATTR ESP8266_TCP_StartDataAcqusition(void)
{
	//START TCP DATA AQUISITION CYCLE
}

void ICACHE_FLASH_ATTR ESP8266_TCP_StopDataAcquisition(void)
{
	//STOP TCP DATA AQUISITION CYCLE
}

void _esp8266_tcp_dns_timer_cb(void* arg)
{
	//ESP8266 DNS CHECK TIMER CALLBACK FUNCTIONS
	//TIME PERIOD = 1 SEC

	//DNS TIMER CB CALLED IE. DNS RESOLUTION DID NOT WORK
	//DO ANOTHER DNS CALL AND RE-ARM THE TIMER

	_esp8266_dns_retry_count++;
	if(_esp8266_dns_retry_count == ESP8266_TCP_DNS_MAX_TRIES)
	{
		//NO MORE DNS TRIES TO BE DONE
		//STOP THE DNS TIMER
		os_timer_disarm(&_esp8266_tcp_dns_timer);
		//CALL USER DNS CB FUNCTION WILL NULL ARGUMENT)
		if(*dns_cb_function != NULL)
		{
			(*dns_cb_function)(NULL);
		}
		os_printf("DNS Max retry exceeded. DNS unsuccessfull\n");
		return;
	}
	os_printf("DNS resolve timer expired. Starting another timer of 1 second...\n");
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

		//CALL USER PROVIDED DNS CB FUNCTION WITH NULL PARAMETER
		if(*dns_cb_function != NULL)
		{
			(*dns_cb_function)(NULL);
		}
		return;
	}

	//DNS GOT IP
	_esp8266_tcp_resolved_host_ip.addr = ipAddr->addr;
	os_printf("hostname : %s, resolved. IP = %d.%d.%d.%d\n", _esp8266_tcp_host_name,
																*((uint8_t*)&_esp8266_tcp_resolved_host_ip.addr),
																*((uint8_t*)&_esp8266_tcp_resolved_host_ip.addr + 1),
																*((uint8_t*)&_esp8266_tcp_resolved_host_ip.addr + 2),
																*((uint8_t*)&_esp8266_tcp_resolved_host_ip.addr + 3));

	//CALL USER PROVIDED DNS CB FUNCTION
	if(*dns_cb_function != NULL)
	{
		(*dns_cb_function)(&_esp8266_tcp_resolved_host_ip);
	}
}
