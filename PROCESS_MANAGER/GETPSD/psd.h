#ifndef __PSD_H__

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

// SOCKET
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define INIT_CONNECT_MSG    "CONNECT"
#define DIS_CONNECT_MSG     "DISCONNECT"

#define SERVERD_PORT    2000

// ===========================================================
// Structure
typedef struct msg{
        char    userName[32];
        char    msgData[10240];
}SocketMessage;

typedef struct client{
    char userName[32];
    char ipAddress[64];
}CLIENT_ADDR;

typedef struct getpsd{

    CLIENT_ADDR  clientAddr;
    int          servSockFd;

}GETPSD_CONF;

// ===========================================================
// Variable

extern GETPSD_CONF		getpsdConf;
extern char    		    myAppName[32];

// ===========================================================
// Function

extern int initPsd();
extern int readConfigData();
extern void sig_interrupt_alarm();
extern int sendSockMsg(char *sendMsg);
extern int initSocket();
extern int getPID_list(char *list, int len);


#endif
