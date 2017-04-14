#ifndef __SERVERD_H__

// COMMON
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

// Signal
#include <signal.h>

#include <unistd.h> // usleep()
#include <ctype.h>  // tolower()

// IPC
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>

// Socket
// #include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

// Select
#include <sys/select.h>

// Thread
#include <pthread.h>

// ===================================================================
// Structure

typedef struct client{
    char userName[32];
    char ipAddress[64];
}CLIENT_TBL;

typedef struct servd{

    key_t   psmanQkey;
    int     servSocketFd;
#define DIS_CONNECT_MSG     "DISCONNECT"
    
#define MAX_CLIENT_USER_TBL 3
    CLIENT_TBL clientTbl[MAX_CLIENT_USER_TBL];
    int clientTblNum;

#define MAX_CLIENT          10
    fd_set  readFd_org;
    int     max_fd;

    pthread_mutex_t *mutex;
   
}ServerdConf;

typedef struct msgq{
    long mtype;
#define MAX_BUFF_SIZE 4096
    char msgBuff[MAX_BUFF_SIZE];
}MsgType;

// ===================================================================
// Variable
#define PSMAN_QKEY      9999
#define SERVERD_PORT    2000

extern ServerdConf serverdConf;
extern char myAppName[32];
extern int  psmanQid;




// ===================================================================
// Function

// serverd_main.c
//
// serverd_init.c
extern void sig_interrupt_alarm();
extern int initServerd();
extern int readConfigData();
extern int initMsgQueue();
extern int initSocket();
extern int rcvSocketMsg();
extern void *thread_main(void *arg);
extern void checkConnection();
extern int sndQueueMsg(char *sndBuff);
extern void disConnect_client(int clientFd);



#endif
