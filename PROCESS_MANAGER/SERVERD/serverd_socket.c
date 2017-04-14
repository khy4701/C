#include "serverd.h"

int rcvSocketMsg(){

    fd_set                readFd_result;
    int                   clilen , ret, i , len = 0;
    char                  readBuff[10240];
    struct timeval        timeout;
    SocketMessage         *recvMsg;

    timeout.tv_sec  = 1;
    timeout.tv_usec = 0;

    FD_ZERO( &readFd_result);

    for (i = 0 ; i < serverdConf.max_fd ; i++){
        if( FD_ISSET( i , &serverdConf.readFd_org))
            FD_SET( i , &readFd_result);
        else
            FD_CLR( i , &readFd_result);
    }

    ret = select( serverdConf.max_fd, &readFd_result, 0, 0, &timeout );
    if (ret < 0){
        fprintf(stderr, "Select Error\n");
        return -1;
    }

    for (i = 0 ; i < serverdConf.max_fd ; i++){
        len = 0;
        memset(readBuff, 0x00, sizeof(readBuff));
        if (FD_ISSET( i , &readFd_result)){

            ret = read( i , readBuff, sizeof(readBuff)-1 );
            if ( ret < 0){
                fprintf( stderr, "read Fail \n");
                return -1;
            }
            if( readBuff[0] == '\0' ) continue;

            recvMsg = (SocketMessage *)readBuff;

			fprintf(stderr, "recvMsg[%s] , readBuff[%s]\n", recvMsg->msgData, readBuff);

            // INIT USER
            if (strcmp( recvMsg->msgData, INIT_CONNECT_MSG) == 0){
                ret = addUserList(recvMsg->userName, i);
                if ( ret < 0 ){
                    fprintf( stderr, "Add User Failed, userName : %s\n", recvMsg->userName);
                    continue;
                }
            }

#if 1
            // CHECK USER
            ret = checkUserList(recvMsg->userName);
            if ( ret < 0){
                fprintf( stderr, "Unknown User Name \n");
                continue;
            }
#endif
#if 1
            if (strcmp( recvMsg->msgData, DIS_CONNECT_MSG) == 0 ){
                disConnect_client( recvMsg->userName, i );
                return -1;
            }

            ret = sndQueueMsg(recvMsg->msgData);
            if (ret < 0){
                fprintf(stderr, "Send Queue Message Failed\n");
                return -1;
            }
#endif
        }
    }

    return 1;
}

void disConnect_client(char *userName, int clientFd){

    int             i =0 , lastFd = -1;
    CLIENT_INFO     *cList;

    if (FD_ISSET( clientFd, &serverdConf.readFd_org)){
        FD_CLR( clientFd, &serverdConf.readFd_org );

        for ( i = 0 ; i < serverdConf.curClientNum ; i++) {

            cList = serverdConf.clientList;
            if( strcmp( cList[i].userName , userName) == 0 )
            {
                memset(cList[i].userName, 0x00, sizeof(cList[i].userName));
                cList[i].fd = 0;
                break;
            }
        }

        fprintf(stderr,"[%s] DisConnected..\n", userName);
    }

    for (i = 0 ; i < serverdConf.max_fd ; i++){
        if ( FD_ISSET( i, &serverdConf.readFd_org) ){
            lastFd = i;
            fprintf(stderr, "MAXFD[%d], lastFD [%d]\n",  serverdConf.max_fd,lastFd);
        }
    }

    if( lastFd == -1){
        serverdConf.max_fd = 0;
        return ;
    }
    serverdConf.max_fd = lastFd + 1;

    return ;
}

int checkUserList(char *userName){

    int i = 0;

    for ( i = 0 ; i < serverdConf.curClientNum ; i++) {
        if( strcmp(serverdConf.clientList[i].userName , userName) == 0 )
            return 1;
    }

    return -1;
}

int addUserList(char *userName, int fd){

    int         i, isAdd = -1;
    CLIENT_INFO *cList;

    cList = serverdConf.clientList;

    for ( i = 0 ; i < MAX_CLIENT_NUM ; i++){
        if( cList[i].userName[0] == '\0' && cList[i].fd == 0 )
            break;
    }

    if ( i == MAX_CLIENT_NUM )
        return -1;

    sprintf(cList[i].userName, userName);
    cList[i].fd = fd;

    serverdConf.curClientNum++;
    fprintf(stderr,"User[%s] Add Success\n", userName);

    return 1;
}
