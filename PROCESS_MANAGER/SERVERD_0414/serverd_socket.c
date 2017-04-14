#include "serverd.h"

int rcvSocketMsg(){

    fd_set                readFd_result;
    int                   clilen , ret, i , len = 0;
    char                  readBuff[10240];
    struct timeval        timeout;

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

#if 1
            ret = read( i , readBuff, sizeof(readBuff)-1 );
            if ( ret < 0){
                fprintf( stderr, "read Fail \n");
                return -1;
            }
#endif
            // READ           
            // printf( "%s", readBuff);
            // Send Queue Message
#if 1
            if( strcmp(readBuff, DIS_CONNECT_MSG) == 0 ){
                disConnect_client(i);
                return -1;
            }

            ret = sndQueueMsg(readBuff);
            if (ret < 0){
                fprintf(stderr, "Send Queue Message Failed\n");
                return -1;
            }
#endif
        }
    }

    return 1;
}

void disConnect_client(int clientFd){
    
    int i =0 , lastFd = -1;

    if (FD_ISSET( clientFd, &serverdConf.readFd_org)){
        FD_CLR( clientFd, &serverdConf.readFd_org );

        fprintf(stderr,"Client DisConnected..\n");
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
