#include "../../libs/include/TCP_client.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

uint64_t SystemMonotonicMS() {
  long ms;
  time_t s;

  struct timespec spec;
  clock_gettime(CLOCK_MONOTONIC, &spec);
  s = spec.tv_sec;
  ms = (spec.tv_nsec / 1000000);

  uint64_t result = s;
  result *= 1000;
  result += ms;

  return result;
}

int main() {
  const char *ip = "127.0.0.1";
  uint16_t port = 5555;
  TCP_client client;
  char message[512];

  printf("Send a message to the sever: ");
  if (fgets(message, sizeof(message - 1), stdin) != NULL) {
    message[strcspn(message, "\n")] = '\0';
  }

  if (TCP_client_init(&client, port, ip) != 0) {
    printf("Failed to initialize client\n");
    return -1;
  }

  if (TCP_client_connect(&client) != 0) {
    printf("Failed to connect to %s:%d\n", ip, port);
  }

  int length = sizeof(message);
  int bytesLeft = length;
  const char *writePtr = message;
  uint64_t now = SystemMonotonicMS();
  uint64_t timeout = now + 5000;

  while (bytesLeft > 0 && now < timeout) {
    now = SystemMonotonicMS();
    int bytesSent = TCP_client_write(&client, (uint8_t *)writePtr, bytesLeft);
    if (bytesSent > 0) {
      writePtr += bytesSent;
      bytesLeft -= bytesSent;
    }
  }

  if (bytesLeft > 0) {
    printf("TIMEOUT ON WRITE!\r\n");
    return 1;
  }

  size_t capacity = 1;
  int bytesRecieved;
  size_t usedSpace = 0;
  char *response = (char *)malloc(capacity + 1);
  if (response == NULL) {
    perror("malloc");
    return -1;
  }

  now = SystemMonotonicMS();
  timeout = now + 5000;

  while (now < timeout) {
    now = SystemMonotonicMS();

    if (usedSpace >= capacity) {
      printf("Capacity before expansion: %zu\n", capacity);
      size_t newCapacity = capacity * 2;
      char *tempBuf = (char *)realloc(response, newCapacity + 1);
      if (tempBuf == NULL) {
        free(response);
        perror("realloc");
        return -1;
      }
      capacity = newCapacity;
      response = tempBuf;
      printf("Capacity after: %zu\n", capacity);
    }

    size_t spaceLeft = capacity - usedSpace;

    bytesRecieved =
        TCP_client_read(&client, (uint8_t *)response + usedSpace, spaceLeft);

    if (bytesRecieved > 0) {
      usedSpace += bytesRecieved;
      continue;
    } else if (bytesRecieved == 0) {
      break;
    }
  }

  if (bytesRecieved > 0) {
    printf("TIMEOUT ON READ!\r\n");
    return 1;
  }

  response[usedSpace] = '\0';
  printf("DATA: %s\n", response);

  free(response);
  TCP_client_dispose(&client);

  return 0;
}
