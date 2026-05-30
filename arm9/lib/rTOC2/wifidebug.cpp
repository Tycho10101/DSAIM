#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include "wifidebug.h"

void rWifiDebug::rInit()
{
    isWifiConnect = false;
}

bool rWifiDebug::rConnect(char *IP, int port)
{
    if (isWifiConnect) return true;
    struct sockaddr_in server;
    if ((dsock = socket(AF_INET, SOCK_STREAM, 0)) < 0 )    
    {    
        return false;    
    }
    memset(&server, 0, sizeof(server));    
    //server.sin_family = AF_INET;    
    server.sin_addr.s_addr = inet_addr(IP);
    server.sin_port = htons(port);
    if (connect(dsock, (struct sockaddr*) &server, sizeof(server)) < 0)
    {
        return false;
    }
    sprintf(dbuffer, "WifiDebug is connected...\n");
    send(dsock, dbuffer, strlen(dbuffer),0);
    isWifiConnect = true;
    return true;
}
    
bool rWifiDebug::rPrint(char *fmt, ...)
{
    if (isWifiConnect == false) 
        return false;
    va_list args;    
    va_start(args, fmt);
    vsnprintf(dbuffer, 5000, fmt, args);
    va_end(args);
    strcat(dbuffer, "\n");
    send(dsock, dbuffer, strlen(dbuffer), 0);
    return true; 
}

void rWifiDebug::rClose()
{
    rPrint("Closing connection...");
    isWifiConnect = false;
    close(dsock);
}

char rWifiDebug::dbuffer[5000];
int rWifiDebug::dsock;
bool rWifiDebug::isWifiConnect = false;
