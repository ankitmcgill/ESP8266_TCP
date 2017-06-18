/*************************************************
* ESP8266 TCP GET LIBRARY
*
* MAY 11 2017
*
* ANKIT BHATNAGAR
* ANKIT.BHATNAGARINDIA@GMAIL.COM
* ***********************************************/

#include "ESP8266_TCP_GET.h"

//LOCAL LIBRARY VARIABLES/////////////////////////////////////
//DEBUG RELATRED
static uint8_t _esp8266_tcp_get_debug;

//TCP RELATED
static struct espconn _esp8266_tcp_get_espconn;
static struct _esp_tcp _esp8266_tcp_get_user_tcp;

//IP / HOSTNAME RELATED
static const char* _esp8266_tcp_get_host_name;
static const char* _esp8266_tcp_get_host_ip;
static ip_addr_t _esp8266_tcp_get_resolved_host_ip;
static const char* _esp8266_tcp_get_host_path;
static uint16_t _esp8266_tcp_get_host_port;

//TIMER RELATED
static volatile os_timer_t _esp8266_tcp_get_dns_timer;
static volatile os_timer_t _esp8266_tcp_get_timer;
static uint32_t _esp8266_tcp_get_timer_interval;
static volatile os_timer_t _esp8266_tcp_reply_timeout_timer;

//COUNTERS
static uint16_t _esp8266_tcp_get_dns_retry_count;
static uint32_t _esp8266_tcp_get_data_acquisition_count;

//TCP OBJECT STATE
static char* _esp8266_tcp_get_get_request_buffer;
static ESP8266_TCP_GET_STATE _esp8266_tcp_get_state;

//CALLBACK FUNCTION VARIABLES
static void (*_esp8266_tcp_get_dns_cb_function)(ip_addr_t*);
static void (*_esp8266_tcp_get_tcp_conn_cb)(void*);
static void (*_esp8266_tcp_get_tcp_discon_cb)(void*);
static void (*_esp8266_tcp_get_tcp_send_cb)(void*);
static void (*_esp8266_tcp_get_tcp_recv_cb)(void*, char*, unsigned short);
static void (*_esp8266_tcp_get_tcp_user_data_ready_cb)(ESP8266_TCP_GET_USER_DATA_CONTAINER*);

//USER DATA RELATED
static ESP8266_TCP_GET_USER_DATA_CONTAINER* _esp8266_user_data_container;
//END LOCAL LIBRARY VARIABLES/////////////////////////////////

void ICACHE_FLASH_ATTR ESP8266_TCP_GET_SetDebug(uint8_t debug_on)
{
    //SET DEBUG PRINTF ON(1) OR OFF(0)
    
    _esp8266_tcp_get_debug = debug_on;
}

void ICACHE_FLASH_ATTR ESP8266_TCP_GET_Initialize(const char* hostname,
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

	_esp8266_tcp_get_host_name = hostname;
	_esp8266_tcp_get_host_ip = host_ip;
	_esp8266_tcp_get_host_port = host_port;
	_esp8266_tcp_get_host_path = host_path;
	_esp8266_tcp_get_timer_interval = tcp_connection_interval_ms;

	_esp8266_tcp_get_data_acquisition_count = 0;
	_esp8266_tcp_get_dns_retry_count = 0;

    //SET DEBUG ON
    _esp8266_tcp_get_debug = 1;
    
	_esp8266_tcp_get_state = ESP8266_TCP_GET_STATE_OK;
}

void ICACHE_FLASH_ATTR ESP8266_TCP_GET_Intialize_Request_Buffer(uint32_t buffer_size)
{
	//ALLOCATE THE ESP8266 TCP GET REQUEST BUFFER

	_esp8266_tcp_get_get_request_buffer = (char*)os_zalloc(buffer_size);

	//GENERATE THE GET STRING USING HOST-NAME & HOST-PATH
	os_sprintf(_esp8266_tcp_get_get_request_buffer, ESP8266_TCP_GET_GET_REQUEST_STRING,
			_esp8266_tcp_get_host_path, _esp8266_tcp_get_host_name);

	if(_esp8266_tcp_get_debug)
	{
	    os_printf("GET STRING : %s\n", _esp8266_tcp_get_get_request_buffer);
	}
}

void ICACHE_FLASH_ATTR ESP8266_TCP_GET_Initialize_UserDataContainer(ESP8266_TCP_GET_USER_DATA_CONTAINER* container)
{
	//SET A LOCAL VARIABLE TO THE USER DATA CONTAINER STRUCTURE

	_esp8266_user_data_container = container;

	//SET THE DATA_FOUND FIELD OF ALL THE EXTRACTED_DATA STRUCTURES IN ESP8266_TCP_USER_DATA_CONTAINER TO
	//DEFAULT VALUE 0 (NOT FOUND). ONLY THE FOUND DATA IN TCP REPLY WOULD BE SET TO 1 (DATA FOUND), REST
	//WILL REMAIN 0 (NOT FOUND)
	uint8_t i=0;
	while(i < _esp8266_user_data_container->tcp_reply_extracted_data_count)
	{
		_esp8266_user_data_container->tcp_reply_extracted_data[i].data_found = 0;
		i++;
	}
}

void ICACHE_FLASH_ATTR ESP8266_TCP_GET_SetDnsServer(char num_dns, ip_addr_t* dns)
{
	//SET DNS SERVER RESOLVE HOSTNAME TO IP ADDRESS
	//MAX OF 2 DNS SERVER SUPPORTED (num_dns)

	if(num_dns == 1 || num_dns == 2)
	{
		espconn_dns_setserver(num_dns, dns);
	}
	return;
}

void ICACHE_FLASH_ATTR ESP8266_TCP_GET_SetCallbackFunctions(void (*tcp_con_cb)(void*),
															void (*tcp_discon_cb)(void*),
															void (tcp_send_cb)(void*),
															void (tcp_recv_cb)(void*, char*, unsigned short),
															void (user_data_ready_cb)(ESP8266_TCP_GET_USER_DATA_CONTAINER*))
{
	//HOOK FOR THE USER TO PROVIDE CALLBACK FUNCTIONS FOR
	//VARIOUS INTERNAL TCP OPERATION
	//SET THE CALLBACK FUNCTIONS FOR THE EVENTS:
	//	(1) TCP CONNECT
	//	(2) TCP DISCONNECT
	//	(3) TCP RECONNECT
	//	(4) TCP SEND
	//	(5) TCP RECEIVE
	//	(6) USER DATA READY
	//IF A NULL FUNCTION POINTER IS PASSED FOR THE CB OF A PARTICULAR
	//EVENT, THE DEFAULT CALLBACK FUNCTION IS CALLED FOR THAT EVENT

	/* TEMPORARY !!!!
	if(user_data_ready_cb == NULL)
	{
		//DO NOT PROCEED. NEED A USER DATA READY CALLBACK
		//THROW ERROR

		_esp8266_tcp_state = ESP8266_TCP_STATE_ERROR;
		return;
	}
	*/
	//TCP DATA READY USER CB
	_esp8266_tcp_get_tcp_user_data_ready_cb = user_data_ready_cb;

	//TCP CONNECT CB
	_esp8266_tcp_get_tcp_conn_cb = tcp_con_cb;

	//TCP DISCONNECT CB
	_esp8266_tcp_get_tcp_discon_cb = tcp_discon_cb;

	//TCP SEND CB
	_esp8266_tcp_get_tcp_send_cb = tcp_send_cb;

	//TCP RECEIVE CB
	_esp8266_tcp_get_tcp_recv_cb = tcp_recv_cb;
}

uint32_t ICACHE_FLASH_ATTR ESP8266_TCP_GET_GetDataAcquisitionInterval(void)
{
	//RETURN ESP8266 TCP TIMER DATA ACQUISITION INTERVAL (MS)

	return _esp8266_tcp_get_timer_interval;
}

const char* ICACHE_FLASH_ATTR ESP8266_TCP_GET_GetSourceHost(void)
{
	//RETURN HOST NAME STRING

	return _esp8266_tcp_get_host_name;
}

const char* ICACHE_FLASH_ATTR ESP8266_TCP_GET_GetSourcePath(void)
{
	//RETURN HOST PATH STRING

	return _esp8266_tcp_get_host_path;
}

ESP8266_TCP_GET_STATE ICACHE_FLASH_ATTR ESP8266_TCP_GET_GetState(void)
{
	//RETURN THE INTERNAL ESP8266 TCP STATE VARIABLE VALUE

	return _esp8266_tcp_get_state;
}

void ICACHE_FLASH_ATTR ESP8266_TCP_GET_ResolveHostName(void (*user_dns_cb_fn)(ip_addr_t*))
{
	//RESOLVE PROVIDED HOSTNAME USING THE SUPPLIED DNS SERVER
	//AND CALL THE USER PROVIDED DNS DONE CB FUNCTION WHEN DONE

	//DONE ONLY IF THE HOSTNAME SUPPLIED IN INITIALIZATION FUNCTION
	//IS NOT NULL. IF NULL, USER SUPPLIED IP ADDRESS IS USED INSTEAD
	//AND NO DNS REOSLUTION IS DONE

	//SET USER DNS RESOLVE CB FUNCTION
	_esp8266_tcp_get_dns_cb_function = user_dns_cb_fn;

	//SET DNS RETRY COUNTER TO ZERO
	_esp8266_tcp_get_dns_retry_count = 0;

	if(_esp8266_tcp_get_host_name != NULL)
	{
		//NEED TO DO DNS RESOLUTION

		//START THE DNS RESOLVING PROCESS AND TIMER
		struct espconn temp;
		_esp8266_tcp_get_resolved_host_ip.addr = 0;
		espconn_gethostbyname(&temp, _esp8266_tcp_get_host_name, &_esp8266_tcp_get_resolved_host_ip, _esp8266_tcp_get_dns_found_cb);
		os_timer_setfn(&_esp8266_tcp_get_dns_timer, (os_timer_func_t*)_esp8266_tcp_get_dns_timer_cb, &temp);
		os_timer_arm(&_esp8266_tcp_get_dns_timer, 1000, 0);
		return;
	}

	//NO NEED TO DO DNS RESOLUTION. USE USER SUPPLIED IP ADDRESS STRING
	_esp8266_tcp_get_resolved_host_ip.addr = ipaddr_addr(_esp8266_tcp_get_host_ip);

	_esp8266_tcp_get_state = ESP8266_TCP_GET_STATE_DNS_RESOLVED;
	//CALL USER SUPPLIED DNS RESOLVE CB FUNCTION
	(*_esp8266_tcp_get_dns_cb_function)(&_esp8266_tcp_get_resolved_host_ip);
}

uint16_t ICACHE_FLASH_ATTR ESP8266_TCP_GET_GetSourcePort(void)
{
	//RETURN HOST REMOTE PORT NUMBER

	return _esp8266_tcp_get_host_port;
}

void ICACHE_FLASH_ATTR ESP8266_TCP_GET_StartDataAcqusition(void)
{
	//START TCP DATA AQUISITION CYCLE
	if(_esp8266_tcp_get_debug)
	{
	    os_printf("ESP8266 TCP : DATA AQUISITION CYCLE START\n");
	}

	_esp8266_tcp_get_data_acquisition_count = 0;

	//CREATE THE BASIC TCP GET REQUEST STRUCTURE AND OBJECTS
	_esp8266_tcp_get_espconn.proto.tcp = &_esp8266_tcp_get_user_tcp;
	_esp8266_tcp_get_espconn.type = ESPCONN_TCP;
	_esp8266_tcp_get_espconn.state = ESPCONN_NONE;

	os_memcpy(_esp8266_tcp_get_espconn.proto.tcp->remote_ip, (uint8_t*)(&_esp8266_tcp_get_resolved_host_ip.addr), 4);
	_esp8266_tcp_get_espconn.proto.tcp->remote_port = _esp8266_tcp_get_host_port;
	_esp8266_tcp_get_espconn.proto.tcp->local_port = espconn_port();

	espconn_regist_connectcb(&_esp8266_tcp_get_espconn, _esp8266_tcp_get_connect_cb);
	espconn_regist_disconcb(&_esp8266_tcp_get_espconn, _esp8266_tcp_get_disconnect_cb);

	if(_esp8266_tcp_get_debug)
	{
	    os_printf("ESP8266 TCP : SETUP TCP OBJECT. STARTING ACQUISITION TIMER WITH INTERVAL = %dms\n", _esp8266_tcp_get_timer_interval);
	}

	//START THE ACQUISITION TIMER
	os_timer_setfn(&_esp8266_tcp_get_timer, (os_timer_func_t*)_esp8266_tcp_get_data_acquisition_timer_cb, NULL);
	os_timer_arm(&_esp8266_tcp_get_timer, _esp8266_tcp_get_timer_interval, 1);

	if(_esp8266_tcp_get_debug)
	{
	    os_printf("ESP8266 TCP : STARTING DATA ACQUISITION CYCLE = %d\n", _esp8266_tcp_get_data_acquisition_count++);
	}

	espconn_connect(&_esp8266_tcp_get_espconn);
}

void ICACHE_FLASH_ATTR ESP8266_TCP_GET_StopDataAcquisition(void)
{
	//STOP TCP DATA AQUISITION CYCLE

	if(_esp8266_tcp_get_debug)
	{
	    os_printf("data acquisition stopped at cycle %d\n", _esp8266_tcp_get_data_acquisition_count);
	}

	_esp8266_tcp_get_data_acquisition_count = 0;

	//DISARM THE TIMER
	os_timer_disarm(&_esp8266_tcp_get_timer);
}

void ICACHE_FLASH_ATTR _esp8266_tcp_get_dns_timer_cb(void* arg)
{
	//ESP8266 DNS CHECK TIMER CALLBACK FUNCTIONS
	//TIME PERIOD = 1 SEC

	//DNS TIMER CB CALLED IE. DNS RESOLUTION DID NOT WORK
	//DO ANOTHER DNS CALL AND RE-ARM THE TIMER

	_esp8266_tcp_get_dns_retry_count++;
	if(_esp8266_tcp_get_dns_retry_count == ESP8266_TCP_GET_DNS_MAX_TRIES)
	{
		//NO MORE DNS TRIES TO BE DONE
		//STOP THE DNS TIMER
		os_timer_disarm(&_esp8266_tcp_get_dns_timer);

		if(_esp8266_tcp_get_debug)
		{
		    os_printf("DNS Max retry exceeded. DNS unsuccessfull\n");
		}

		_esp8266_tcp_get_state = ESP8266_TCP_GET_STATE_ERROR;
		//CALL USER DNS CB FUNCTION WILL NULL ARGUMENT)
		if(*_esp8266_tcp_get_dns_cb_function != NULL)
		{
			(*_esp8266_tcp_get_dns_cb_function)(NULL);
		}
		return;
	}

	if(_esp8266_tcp_get_debug)
	{
	    os_printf("DNS resolve timer expired. Starting another timer of 1 second...\n");
	}

	struct espconn *pespconn = arg;
	espconn_gethostbyname(pespconn, _esp8266_tcp_get_host_name, &_esp8266_tcp_get_resolved_host_ip, _esp8266_tcp_get_dns_found_cb);
	os_timer_arm(&_esp8266_tcp_get_dns_timer, 1000, 0);
}

void ICACHE_FLASH_ATTR _esp8266_tcp_get_dns_found_cb(const char* name, ip_addr_t* ipAddr, void* arg)
{
	//ESP8266 TCP DNS RESOLVING DONE CALLBACK FUNCTION

	//DISABLE THE DNS TIMER
	os_timer_disarm(&_esp8266_tcp_get_dns_timer);

	if(ipAddr == NULL)
	{
		//HOST NAME COULD NOT BE RESOLVED
		if(_esp8266_tcp_get_debug)
		{
		    os_printf("hostname : %s, could not be resolved\n", _esp8266_tcp_get_host_name);
		}

		_esp8266_tcp_get_state = ESP8266_TCP_GET_STATE_ERROR;
		//CALL USER PROVIDED DNS CB FUNCTION WITH NULL PARAMETER
		if(*_esp8266_tcp_get_dns_cb_function != NULL)
		{
			(*_esp8266_tcp_get_dns_cb_function)(NULL);
		}
		return;
	}

	//DNS GOT IP
	_esp8266_tcp_get_resolved_host_ip.addr = ipAddr->addr;
	if(_esp8266_tcp_get_debug)
	{
	    os_printf("hostname : %s, resolved. IP = %d.%d.%d.%d\n", _esp8266_tcp_get_host_name,
																    *((uint8_t*)&_esp8266_tcp_get_resolved_host_ip.addr),
																    *((uint8_t*)&_esp8266_tcp_get_resolved_host_ip.addr + 1),
																    *((uint8_t*)&_esp8266_tcp_get_resolved_host_ip.addr + 2),
																    *((uint8_t*)&_esp8266_tcp_get_resolved_host_ip.addr + 3));
	}

	_esp8266_tcp_get_state = ESP8266_TCP_GET_STATE_DNS_RESOLVED;

	//CALL USER PROVIDED DNS CB FUNCTION
	if(*_esp8266_tcp_get_dns_cb_function != NULL)
	{
		(*_esp8266_tcp_get_dns_cb_function)(&_esp8266_tcp_get_resolved_host_ip);
	}
}

void ICACHE_FLASH_ATTR _esp8266_tcp_get_connect_cb(void* arg)
{
	//TCP CONNECT CALLBACK

	if(_esp8266_tcp_get_debug)
	{
	    os_printf("ESP8266 TCP : TCP CONNECTED\n");
	}

	//GET THE NEW USER TCP CONNECTION
	struct espconn *pespconn = arg;

	//REGISTER SEND AND RECEIVE CALLBACKS
	espconn_regist_sentcb(pespconn, _esp8266_tcp_get_send_cb);
	espconn_regist_recvcb(pespconn, _esp8266_tcp_get_receive_cb);

	//SEND USER DATA (GET REQUEST)
	char* pbuf = (char*)os_zalloc(2*2048);
	os_sprintf(pbuf, _esp8266_tcp_get_get_request_buffer,
						pespconn->proto.tcp->remote_ip[0], pespconn->proto.tcp->remote_ip[1],
						pespconn->proto.tcp->remote_ip[2], pespconn->proto.tcp->remote_ip[3]);

	//SEND DATA AND DEALLOCATE BUFFER
	espconn_sent(pespconn, pbuf, os_strlen(pbuf));
	os_free(pbuf);

	//CALL USER CALLBACK IF NOT NULL
	if(_esp8266_tcp_get_tcp_conn_cb != NULL)
	{
		(*_esp8266_tcp_get_tcp_conn_cb)(arg);
	}
}

void ICACHE_FLASH_ATTR _esp8266_tcp_get_disconnect_cb(void* arg)
{
	//TCP DISCONNECT CALLBACK

	if(_esp8266_tcp_get_debug)
	{
	    os_printf("ESP8266 TCP : TCP DISCONNECTED\n");
	}

	//CALL USER CALLBACK IF NOT NULL
	if(_esp8266_tcp_get_tcp_discon_cb != NULL)
	{
		(*_esp8266_tcp_get_tcp_discon_cb)(arg);
	}
}

void ICACHE_FLASH_ATTR _esp8266_tcp_get_send_cb(void* arg)
{
	//TCP SENT DATA SUCCESSFULLY CALLBACK

	if(_esp8266_tcp_get_debug)
	{
	    os_printf("ESP8266 TCP : TCP DATA SENT\n");
	}

	//SET AND THE TCP GET REPLY TIMEOUT TIMER
	os_timer_setfn(&_esp8266_tcp_reply_timeout_timer, (os_timer_func_t*)_esp8266_tcp_get_receive_timeout_cb, NULL);
	os_timer_arm(&_esp8266_tcp_reply_timeout_timer, ESP8266_TCP_GET_REPLY_TIMEOUT_MS, 0);
	if(_esp8266_tcp_get_debug)
	{
		os_printf("ESP8266 TCP : Started 5 second reply timeout timer\n");
	}

	//CALL USER CALLBACK IF NOT NULL
	if(_esp8266_tcp_get_tcp_send_cb != NULL)
	{
		(*_esp8266_tcp_get_tcp_send_cb)(arg);
	}
}

void ICACHE_FLASH_ATTR _esp8266_tcp_get_receive_cb(void* arg, char* pusrdata, unsigned short length)
{
	//TCP RECEIVED DATA CALLBACK

	if(_esp8266_tcp_get_debug)
	{
	    os_printf("ESP8266 TCP : TCP DATA RECEIVED\n");
	}

	//PROCESS INCOMING TCP DATA

	//CHECK FOR USER DATA IN THE REPLY
	char* ptr;
	uint8_t counter = 0;

	while(counter < _esp8266_user_data_container->tcp_reply_extracted_data_count)
	{
		if((ptr = strstr(pusrdata, (_esp8266_user_data_container->tcp_reply_extracted_data[counter]).extracted_data_start_match_string)) != NULL)
		{
			//DATA FOUND
			char* data = (char*)os_zalloc(256);

			//ADVANCE THE POINTER TO DATA LOCATION
			ptr += (_esp8266_user_data_container->tcp_reply_extracted_data[counter]).extracted_data_offset_from_match_string;

			//LOOP TO GET DATA
			if((_esp8266_user_data_container->tcp_reply_extracted_data[counter]).extracted_data_char_len == 0)
			{
				//NEED TO LOOP TILL WE FIND DATA TERMINATING CHARACTER
				uint8_t i=0;
				while(ptr[i] != (_esp8266_user_data_container->tcp_reply_extracted_data[counter]).extracted_data_terminating_char)
				{
					data[i] = ptr[i];
					i++;
				}
			}
			else
			{
				//NEED TO LOOP A FIXED NUMBER OF TIMER
				uint8_t i;
				for(i=0; i<(_esp8266_user_data_container->tcp_reply_extracted_data[counter]).extracted_data_char_len; i++)
				{
					data[i] = ptr[i];
				}
			}
			//ASSIGN THE DATA POINTER
			strcpy((_esp8266_user_data_container->tcp_reply_extracted_data[counter]).extracted_data, data);
			(_esp8266_user_data_container->tcp_reply_extracted_data[counter]).data_found = 1;

			//FREE THE DATA BUFFER
			os_free(data);
			if(_esp8266_tcp_get_debug)
			{
			    os_printf("data found = %s\n", (_esp8266_user_data_container->tcp_reply_extracted_data[counter]).extracted_data);
			}
		}
		counter++;
	}

	//CALL USER CALLBACK IF NOT NULL
	if(_esp8266_tcp_get_tcp_recv_cb != NULL)
	{
		(*_esp8266_tcp_get_tcp_recv_cb)(arg, pusrdata, length);
	}

	//CHECK FOR PACKET ENDING CONDITION
	if(strstr(pusrdata, _esp8266_user_data_container->tcp_reply_packet_terminating_chars) != NULL)
	{
		//END OF PACKET
		//DISCONNECT TCP CONNECTION
		espconn_disconnect(&_esp8266_tcp_get_espconn);

		//STOP TCP GET REPLY TIMEOUT TIMER
		os_timer_disarm(&_esp8266_tcp_reply_timeout_timer);
		if(_esp8266_tcp_get_debug)
		{
			os_printf("ESP8266 TCP : TCP get reply timeout timer stopped\n");
		}

		//CALL USER SPECIFIED DATA READY CALLBACK
		(*_esp8266_tcp_get_tcp_user_data_ready_cb)(_esp8266_user_data_container);
	}
}

void ICACHE_FLASH_ATTR _esp8266_tcp_get_receive_timeout_cb(void)
{
	//CALLBACK FOR TCP GET REPLY TIMEOUT TIMER
	//IF CALLED => TCP GET REPLY NOT RECEIVED IN SET TIME
	if(_esp8266_tcp_get_debug)
	{
		os_printf("ESP8266 TCP : TCP get reply timeout !\n");
	}

	//DISCONNECT THE TCP CONNECTION TO END THE CURRENT TRANSACTION
	espconn_disconnect(&_esp8266_tcp_get_espconn);

	////CALL USER SPECIFIED DATA READY CALLBACK WITH NULL ARGUMENT
	(*_esp8266_tcp_get_tcp_user_data_ready_cb)(NULL);
}

void ICACHE_FLASH_ATTR _esp8266_tcp_get_data_acquisition_timer_cb(void* arg)
{
	//ESP8266 DATA ACQUISITION TIMER CALLABCK
	if(_esp8266_tcp_get_debug)
	{
	    os_printf("ESP8266 TCP : STARTING DATA ACQUISITION CYCLE = %d\n", _esp8266_tcp_get_data_acquisition_count++);
	}

	//INITIATE A NEW TCP CONNECTION
	espconn_connect(&_esp8266_tcp_get_espconn);
}
