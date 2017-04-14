#include "psmanager.h"

int rcvQueueMsg(char *readBuff){

	MsgType  rcvMsg;
	ssize_t  readBytes = 0;
    size_t   msgSize;
    int      ret;

	msgSize   = sizeof(MsgType) - sizeof(rcvMsg.mtype);
	readBytes = msgrcv( (key_t)myQid , &rcvMsg, msgSize, 1, IPC_NOWAIT);

	if (readBytes > 0){
		sprintf(readBuff, "%s", rcvMsg.msgBuff);
		return 1;
	}

	if (errno != ENOMSG){
		fprintf(stderr, "MsgRcv() Error\n");
		return -1;
	}

	return -1;
}
