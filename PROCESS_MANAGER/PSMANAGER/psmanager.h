#ifndef __PSMANAGER_H__

// COMMON
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

// Signal
#include <signal.h>

// IPC
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>

// Shared Memory
//#include <sys/ipc.h>
//#include <sys/types.h>
#include <errno.h>
#include <sys/shm.h>

#define MY_QKEY    	9999 
#define	SHM_KEY		5678
#define MEM_SIZE	50000

#define SHM_EXIST	1
#define SHM_CREATE	2


// ===================================================================
// Structure

typedef struct {

    char *shm_addr;

}PSMANAGER_CONF;

typedef struct msgq{
    long mtype;
#define MAX_BUFF_SIZE 4096
    char msgBuff[MAX_BUFF_SIZE];
}MsgType;

// ===================================================================
// Variable
extern PSMANAGER_CONF psmConf;
extern char     myAppName[32];
extern int      myQid;
extern FILE     *result_fp;

// ===================================================================
// Function
extern int initMsgQueue();
extern int initPsm();
extern void sig_interrupt_alarm();
extern void sig_stop_alarm();
extern int initSharedMemory();
extern int rcvQueueMsg(char *readBuff);
extern int writeShmMemory(char *readBuff);


#endif
