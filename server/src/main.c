#include "../include/http.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define BACKLOG 15


HTTPServer_Initiate(&_Server->httpServer, WeatherServer_OnHTTPConnection);

_Server->instances = LinkedList_create();

_Server->task = smw_createTask(_Server, WeatherServer_TaskWork);
int main(void) {
}
