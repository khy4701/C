#include "psmanager.h"

PSMANAGER_CONF psmConf = {};
FILE           *result_fp;

int main(){

    int     ret = 0;
    time_t	now, old;
    char    readBuff[10240];

    result_fp = fopen("result.dat", "a+");

    // 01. INIT & LOAD CONFIG
    ret = initPsm();
    if (ret < 0){
        fprintf(stderr, " initPsm() is failed\n");
        exit(1);
    }

    now = time(0);
    old = now;

    while(1){

        now = time(0);
        if ( now == old )
            continue;

        memset(readBuff, 0x00, sizeof(readBuff));
        ret = rcvQueueMsg(readBuff);
        if (ret < 0){
            usleep(1000);
            old = now;
            continue;
        }

        sprintf( psmConf.shm_addr, "%s", readBuff);

        if( result_fp != NULL) 
            fprintf( result_fp, "%s\n\n", readBuff);

        //writeShmMemory(readBuff);
                
        old = now;
    }

    fclose(result_fp);

    return 1;
}

int writeShmMemory(char *readBuff){

    int len;

    len = strlen( psmConf.shm_addr );
    len = sprintf( psmConf.shm_addr + len, "%s", readBuff) ;

    return 1;
}
