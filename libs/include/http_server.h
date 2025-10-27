#ifndef __http_client_h_
#define __http_client_h_

#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include "sm_worker.h"
#include "TCP_client.h"

#ifndef http_max_url_length
  #define http_max_url_length 1024
#endif


typedef enum
{
  read_firstline,
  read_headers,
  read_body,
  callback

} Http_Server_State;

typedef struct
{
	Http_State state; /* TCP Client state*/
	SMW_Task* task;
	char url[http_max_url_length + 1];
	uint64_t timeout;
	
	void (*callback)(const char* _Event, const char* _Response);

	uint64_t timer;

  TCP_client tcp;

	bool is_connected;

} Http_Client;

int http_client_init(const char* _url, Http_Client** _Client_Ptr);

int http_client_get(const char* _url, uint64_t _timeout, void (*_callback)(const char* _event, const char* _res));

#endif
