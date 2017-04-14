#include "serverd.h"

ServerdConf serverdConf = {};

int main()
{
    int             ret = 0;
    int             pid;
    pthread_t       thrdId;
    pthread_attr_t  thrAttr;

    // 01. Initial
    ret = initServerd();
    if (ret < 0){
        // Change to logPrint();
        fprintf(stderr, "Initial Serverd Failed\n");
        exit(1);
    }

    pthread_attr_init(&thrAttr);
    pthread_mutex_init( serverdConf.mutex , NULL);

    ret = pthread_create(&thrdId, &thrAttr, thread_main, NULL);  
    if ( ret < 0){
        fprintf(stderr, "pthread_create() is Failed\n");
        exit(1);
    }

    // 02. Receive Socket & Send Queue Message 
    while(1){

        // 02-01. Receive Socket Message
#if 1
        pthread_mutex_lock( serverdConf.mutex );
        ret = rcvSocketMsg();
        pthread_mutex_unlock( serverdConf.mutex );

        if (ret < 0){
            usleep(1000);
            continue;
        }
#endif

        // 02-02. Check Connection
		checkConnection();
    } 

    return 1;
}

void *thread_main(void *arg){
    int clientSockFd = 0;
    int clilen;
    struct sockaddr_in    cli_addr;

    while(1){

        clilen = sizeof(cli_addr);

        clientSockFd = accept(serverdConf.servSocketFd, (struct sockaddr *) &cli_addr, &clilen );
        if (clientSockFd < 0){
            fprintf(stderr, "Error to Accept() \n");           
            return NULL;
        }
        pthread_mutex_lock( serverdConf.mutex );
        FD_SET ( clientSockFd, &serverdConf.readFd_org );

        serverdConf.max_fd = clientSockFd + 1;
        pthread_mutex_unlock( serverdConf.mutex );
    }
    return NULL;
}

void checkConnection(){

	return ;

}
