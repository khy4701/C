#include "psd.h"

GETPSD_CONF		getpsdConf = {};

int main(){

    int     ret = 0, len;
	time_t	now, old;
	char 	cmdBuff[10240];

	// 01. INIT & LOAD CONFIG
	ret = initPsd();
	if (ret < 0){
		fprintf(stderr, " initPsd() is failed\n");
		exit(1);
	}

	now = time(0);
	old = now;

	while(1){

		now = time(0);
		if ( now == old )
			continue;

		// 02. GETPID LIST
#if 1
        memset(cmdBuff, 0x00 , sizeof(cmdBuff));
        len = 0;

        len += sprintf(cmdBuff, " IP[%s] User[%s] \n",  getpsdConf.clientAddr.ipAddress, 
                getpsdConf.clientAddr.userName);

        len += sprintf(cmdBuff+len,"========================================\n");

		len = getPID_list(cmdBuff, len);
        if (len < 0){
			fprintf(stderr, "getPID_list() \n");
            exit(1);
        }

        len += sprintf(cmdBuff+len,"========================================\n");

 //       printf("%s", cmdBuff);

#endif		
#if 1
		// 03. SEND LIST to SERVERD
		ret = sendSockMsg(cmdBuff);
		if (ret < 0){
			fprintf(stderr, "sendSockMsg is failed\n");
			exit(1);
		}
        fprintf(stderr, "Message Send Success\n");
#endif
       
		old = now;
	}

	return 1;
}

int getPID_list(char *list, int len){

    int  ret;
    char command[1024], fileName[32];
    char readBuff[128];
    FILE *fp;

    sprintf(fileName, "%s", "temp.txt"); 
    sprintf(command, "ps -ef > %s", fileName);

    ret = system(command);
    if (ret < 0){
        fprintf(stderr, "system() is failed CMD[%s] \n", command);
        return -1;
    }

    fp = fopen("temp.txt", "r+");
    if (fp == NULL){
        fprintf(stderr, "fopen [%s] is failed \n", fileName);
        return -1;
    }
#if 1

    while( fgets(readBuff, sizeof(readBuff), fp) != NULL){
        len += sprintf( list + len, "%s", readBuff);
    }
    
    len += sprintf(list + len, "%s", "\n\n");
#endif

    fclose(fp);
    return len;
}
