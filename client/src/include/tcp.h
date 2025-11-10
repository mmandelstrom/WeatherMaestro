#ifndef __TCP_H__
#define __TCP_H__

#define _POSIX_C_SOURCE 200809L /* This must be defined before netdb.h */

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>

#define BACKLOG 15
#define MAX_CLIENTS 15


/* ******************************************************************* */
/* *************************** TCP CLIENT **************************** */
/* ******************************************************************* */

typedef enum {
  CLIENT_STATE_INIT, 
  CLIENT_STATE_CONNECTING,
  CLIENT_STATE_READING,
  CLIENT_STATE_WRITING,
  CLIENT_STATE_DISPOSING,
  CLIENT_STATE_ERROR
} TCPClientState;


#endif
