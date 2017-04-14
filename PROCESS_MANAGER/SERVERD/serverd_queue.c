#include "serverd.h"

 MsgType     sndMsg;
int sndQueueMsg(char *sndBuff){

    size_t      msgSize;
    int         ret = 0, len ;

    memset( &sndMsg, 0x00, sizeof(sndMsg));
    sndMsg.mtype = 1;

    memcpy( sndMsg.msgBuff, sndBuff , strlen(sndBuff));

    msgSize = sizeof(MsgType) - sizeof(sndMsg.mtype);
    fprintf(stderr, "msgSize[%ld], psmanQid[%d], sendMsg[%ld]\n", msgSize, psmanQid, strlen(sndMsg.msgBuff));

    len = strlen(sndMsg.msgBuff);
    sndMsg.msgBuff[len] = '\0';
    ret = msgsnd( psmanQid, &sndMsg, msgSize, IPC_NOWAIT);

    if (ret == -1){

        fprintf(stderr, "msgsnd() is failed , errno[%d]\n", errno);
        switch (errno){
            case EAGAIN:
                fprintf(stderr, " EAGAIN\n");
                break;

            case EACCES:
                fprintf(stderr, " EACCES\n");
                break;

            case EFAULT:
                fprintf(stderr, " EFAULT\n");
                break;

            case EINTR:
                fprintf(stderr, " EINTR\n");
                break;
            case EINVAL :
                fprintf(stderr, " EINVAL\n");
                break;
            case ENOMEM:
                fprintf(stderr, " ENOMEM\n");
                break;
        }

        return -1;
    }

    return 1;
}
