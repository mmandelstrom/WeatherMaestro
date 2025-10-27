#include "../include/http_client.h"

/*================ Internal Functions ================*/

void http_client_work(void* _Context, uint64_t _MonTime);
void http_client_dispose(Http_Client** _ClientPtr);

/*====================================================*/

int http_client_init(const char* _url, Http_Client** _Client_Ptr)
{
  if (_url == NULL || _Client_Ptr == NULL)
    return -1;

  if (strlen(_url) > http_max_url_length)
    return -2;

  Http_Client* _Client = (Http_Client*)malloc(sizeof(Http_Client));
  if (_Client == NULL)
    return -3;
  
	_Client->state = http_client_state_init;
	_Client->task = smw_create_task(_Client, http_client_work);

	_Client->callback = NULL;
	_Client->timer = 0;
	_Client->is_connected = 0;

	strcpy(_Client->url, _url);

  /* Call TCP client init*/
  tcp_client_init(&_Client->tcp, );

  if (tcp_client_connect(&_Client->tcp, &_Client->url, &_Client->port) < 0) {
    perror("HTTP: TCP connect failed");
    tcp_client_dispose(&_Client->tcp);
    free(_Client);
    return -4;
  }

  *(_Client_Ptr) = _Client;

  return 0;
}

int http_client_get(const char* _URL, uint64_t _Timeout, void (*_Callback)(const char* _Event, const char* _Response))
{
	Http_Client* client = NULL;
	if(http_client_init(_URL, &client) != 0)
		return -1;

	client->timeout = _Timeout;
	client->callback = _Callback;

	return 0;
}

Http_State http_client_work_init(Http_Client* _Client)
{
	return http_client_state_connect;
}

Http_State http_client_work_connect(Http_Client* _Client)
{
	return http_client_state_connecting;
}

Http_State http_client_work_connecting(Http_Client* _Client)
{
	if(_Client->is_connected)
		return http_client_state_writing;

	return http_client_state_connecting;
}

Http_State http_client_work_writing(Http_Client* _Client)
{
	return http_client_state_reading;
}

Http_State http_client_work_reading(Http_Client* _Client)
{	
	return http_client_state_done;
}

Http_State http_client_work_done(Http_Client* _Client)
{
	if(_Client->callback != NULL)
		_Client->callback("RESPONSE", "Det här är responsen!");

	return http_client_state_dispose;
}

void http_client_work(void* _context, uint64_t _mon_time)
{
	Http_Client* _Client = (Http_Client*)_context;

	if(_Client->timer == 0)
	{
		_Client->timer = _mon_time;
	}
	else if(_mon_time >= _Client->timer + _Client->timeout)
	{
		if(_Client->callback != NULL)
			_Client->callback("TIMEOUT", NULL);

		http_client_dispose(&_Client);
		return;
	}

	printf("%i > %s\r\n", _Client->state, _Client->url);

	switch(_Client->state)
	{
		case http_client_state_init:
		{
			_Client->state = http_client_work_init(_Client);
		} break;
		
		case http_client_state_connect:
		{
			_Client->state = http_client_work_connect(_Client);
		} break;
		
		case http_client_state_connecting:
		{
			_Client->state = http_client_work_connecting(_Client);
		} break;
		
		case http_client_state_writing:
		{
			_Client->state = http_client_work_writing(_Client);
		} break;
		
		case http_client_state_reading:
		{
			_Client->state = http_client_work_reading(_Client);
		} break;
		
		case http_client_state_done:
		{
			_Client->state = http_client_work_done(_Client);
		} break;
		
		case http_client_state_dispose:
		{
			http_client_dispose(&_Client);
		} break;
		
	}
	
}

void http_client_dispose(Http_Client** _Client_Ptr)
{
	if(_Client_Ptr == NULL || *_Client_Ptr == NULL)
		return;

	Http_Client* _Client = *_Client_Ptr;

	if(_Client->task != NULL)
		smw_destroy_task(_Client->task);

	free(_Client);

	*_Client_Ptr = NULL;
}
