#include "psd.h"

int sendSockMsg(char *sendMsg){

    SocketMessage   sndMessage;
    int         ret = 0;
    
    memset(&sndMessage, 0x00, sizeof(sndMessage));

    sprintf(sndMessage.msgData,  sendMsg);
    sprintf(sndMessage.userName, getpsdConf.clientAddr.userName);

    fprintf(stderr, "SEND MSG : data- %s , userName - %s \n", sndMessage.msgData,sndMessage.userName);

    ret = write(getpsdConf.servSockFd, (void *)&sndMessage, sizeof(sndMessage));
    if (ret < 0){
        fprintf(stderr, " write() is failed \n");
        return -1;
    }

    return 1;
}
