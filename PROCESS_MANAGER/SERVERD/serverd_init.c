#include "serverd.h"

char        myAppName[32];
int         psmanQid;

int initServerd(){

    int     ret = 0;

    // 01. Register Signal Alarm
    signal( SIGINT, (void *)sig_interrupt_alarm ); 

    // 02. Get System Environment
    sprintf(myAppName, "%s", "serverd");

    // 03. Read ServerD Config ( IP Address )
    ret = readConfigData();
    if (ret < 0){
        fprintf( stderr, "readConfigData() is failed \n");
        return -1;
    }

    // 04. Init Message Queue
    ret = initMsgQueue();
    if (ret < 0){
        fprintf( stderr, "initMsgQueue() is failed \n");
        return -1;
    }

    // 05. Init Sokcet 
    ret = initSocket();
    if (ret < 0){
        fprintf( stderr, "initSocket() is failed \n");
        return -1;
    }

    // 06. Init Mutex()
    serverdConf.mutex    = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));

    // 07. Init Client List;
    memset(serverdConf.clientList, 0x00, sizeof(CLIENT_INFO) * MAX_CLIENT_NUM);
    serverdConf.curClientNum = 0;

  return 1;  
}

void sig_interrupt_alarm(){

    fprintf( stderr, "SIGINT[%d] is occured\n", SIGINT);
    exit(1);

}

int readConfigData(){

    int         i;
    FILE        *fp  =  NULL;
    char        fName[32];
    char        readBuff[128];
    char        userName[32], ipAddress[64];
    int         readAddress_flag = 0;
    
    CLIENT_TBL  *clientTbl;

    sprintf(fName, "%s.dat", myAppName);

    fp = fopen( fName, "r+");
    if (fp == NULL){
        fprintf( stderr, "File Open Failed\n");
        return -1;
    }

    clientTbl = serverdConf.clientTbl;
    // fgets() : \n 문자까지 읽어옴 
    while (  fgets(readBuff, sizeof(readBuff), fp) != NULL ){

        if (readBuff[0] == '#' || readBuff[0] == '\n')
            continue;

        if (strcmp(readBuff, "[IP_ADDRESS]\n") == 0 ){
            readAddress_flag = 1;
            continue;
        }

        if (readAddress_flag){
            sscanf(readBuff, "%s = %s\n", userName, ipAddress);
            
            if (serverdConf.clientTblNum >= MAX_CLIENT_USER_TBL )
                continue;
            
            sprintf(clientTbl[serverdConf.clientTblNum].userName , userName);
            sprintf(clientTbl[serverdConf.clientTblNum].ipAddress, ipAddress);

            serverdConf.clientTblNum++;
        }
    }

    fclose(fp);

    return 1;
}

int initMsgQueue(){

    struct msqid_ds       statBuf, tmpBuf;

    if ( (psmanQid = msgget( PSMAN_QKEY, IPC_CREAT | 0666)) == -1 ){
        fprintf(stderr, "Message Get Failed PSMANAGER QKey[%lld]\n",  serverdConf.psmanQkey);
        return -1;
    }

#if 1
    if( msgctl( psmanQid, IPC_STAT, &tmpBuf) == -1)
    {
        fprintf(stderr, "Fail to get msgctl[IPC_STAT]\n");
        return(-1);
    }


    if ( msgctl( psmanQid, IPC_SET, &tmpBuf) == -1 ){
        fprintf(stderr, "Fail to get msgctl[IPC_SET]\n");
        return -1;
    }
#endif
    return 1;
}

int initSocket(){

    struct sockaddr_in      serv_addr;
    int                     portNum;

    serverdConf.servSocketFd = socket( AF_INET, SOCK_STREAM, 0 );
    if (serverdConf.servSocketFd < 0){
        fprintf(stderr,"%s\n","Error Opening Socket");
        return -1;
    }

    bzero( (char *)&serv_addr, sizeof(serv_addr) );

    serv_addr.sin_family        = AF_INET;
    serv_addr.sin_addr.s_addr   = INADDR_ANY;
    serv_addr.sin_port          = htons(SERVERD_PORT);

    if (bind(serverdConf.servSocketFd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0){
        fprintf(stderr, "%s\n", "[initSocket] Binding Error!");
        return -1;
    }

    listen( serverdConf.servSocketFd, 5 );

    FD_ZERO(&serverdConf.readFd_org); 

    return 1;
}
