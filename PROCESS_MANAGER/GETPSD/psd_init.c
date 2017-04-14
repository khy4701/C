#include "psd.h"

char    		myAppName[32];

int initPsd(){

	int     ret = 0;

	// 01. Register Signal Alarm
	signal (SIGINT, (void *)sig_interrupt_alarm);

	// 02. Get System Name
	sprintf(myAppName, "%s", "getpsd");

	// 03. Read GETPSD CONFIG DATA
	ret =  readConfigData();
	if (ret < 0){
		fprintf( stderr, "readConfigData() is failed \n");
		return -1;
	}

    // 04. Setting Socket
    ret = initSocket();
	if (ret < 0){
		fprintf( stderr, "initSocket() is failed \n");
		return -1;
	}

	return 1;
}

void sig_interrupt_alarm(){

	char sendBuff[32];
    int  ret = 0;

    fprintf( stderr, "SIGINT[%d] is occured\n", SIGINT);
    sprintf(sendBuff, "%s", "DISCONNECT");
    
    // 강제 종료시 Server 로 메시지 전송 
//    ret = sendSockMsg(sendBuff);
    ret = sendSockMsg(DIS_CONNECT_MSG);
    if (ret < 0){
        fprintf(stderr, " Send Error to Message[DISCONNECT]\n");
    }

    sleep(1);
    close(getpsdConf.servSockFd);
	exit(1);
}

int readConfigData(){

	int         i;
	FILE        *fp  =  NULL;
	char        fName[32];
	char        readBuff[128];
	char        userName[32], ipAddress[64];
	int         readAddress_flag = 0;
	CLIENT_ADDR *clientAddr;

	sprintf(fName, "%s.dat", myAppName);

	fp = fopen( fName, "r+");
	if (fp == NULL){
		fprintf( stderr, "File Open Failed[%s]\n", fName);
		return -1;
	}

	clientAddr = &getpsdConf.clientAddr;
	while (  fgets(readBuff, sizeof(readBuff), fp) != NULL ){

		if (readBuff[0] == '#' || readBuff[0] == '\n')
			continue;

		if (strcmp(readBuff, "[IP_ADDRESS]\n") == 0 ){
			readAddress_flag = 1;
			continue;
		}

		if (readAddress_flag){
			sscanf(readBuff, "%s = %s\n", userName, ipAddress);

			sprintf(clientAddr->userName , userName);
			sprintf(clientAddr->ipAddress, ipAddress);
		}
	}

	fclose(fp);

	return 1;
}

int initSocket(){

    int                  ret;
    struct sockaddr_in   serv_addr;
    struct hostent       *server;
        
    getpsdConf.servSockFd = socket(AF_INET, SOCK_STREAM, 0);
    if (getpsdConf.servSockFd < 0){
        fprintf(stderr, "socket() is failed\n");
        return -1;
    }

    // CONFIG
   server = gethostbyname(getpsdConf.clientAddr.ipAddress);
   if (server == NULL){
        fprintf(stderr, "getHostbyname() is failed\n");
        return -1;
   }

   bzero( &serv_addr, sizeof(serv_addr) );
   serv_addr.sin_family = AF_INET;
   bcopy( server->h_addr, &serv_addr.sin_addr.s_addr, server->h_length);
   serv_addr.sin_port   = htons(SERVERD_PORT);

   // CONNET
   if (connect(getpsdConf.servSockFd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0){
       fprintf(stderr, " connect() is failed \n");
       return -1;
   }

   sendSockMsg(INIT_CONNECT_MSG); 

   return 1;
}
