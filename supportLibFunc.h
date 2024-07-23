#pragma once

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/time.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PORT 8000
#define BUFFER_SIZE 512
#define MAX_PATH_LENGTH 256

#ifdef _WIN32
    DWORD waitingTime = 10000;//10 s
#else
    struct timeval waitingTime;
    waitingTime.tv_sec = 10;//10 s
    waitingTime.tv_usec = 0;
#endif