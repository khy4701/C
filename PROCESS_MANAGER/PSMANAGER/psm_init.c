#include "psmanager.h"

char    		myAppName[32];
int             myQid;

int initPsm(){

    int     ret = 0;

	// 01. Register Signal Alarm
	signal (SIGINT, (void *)sig_interrupt_alarm);
	signal (SIGTSTP, (void *)sig_stop_alarm);

    // 02. Init Message Queue
    ret = initMsgQueue();
    if (ret < 0){
        fprintf( stderr, "initMsgQueue() is failed \n");
        return -1;
    }

    // 03. Init Shared Memory
    ret = initSharedMemory();
    if (ret < 0){
        fprintf( stderr, "initMsgQueue() is failed \n");
        return -1;
    }

    return 1;
}

void sig_interrupt_alarm(){

	fprintf( stderr, "SIGINT[%d] is occured\n", SIGINT);

    if (result_fp != NULL) {
        fflush(result_fp);
        fclose(result_fp);
    }

	exit(1);
}

void sig_stop_alarm(){

    fprintf(stderr, "%s\n", psmConf.shm_addr);
}

int initMsgQueue(){

    if ( (myQid = msgget( MY_QKEY, IPC_CREAT | 0666)) == -1 ){
        fprintf(stderr, "Message Get Failed PSMANAGER QKey[%d]\n", myQid );
        return -1;
    }

    return 1;
}

int initSharedMemory(){

	int shm_id;

	shm_id = shmget(  (key_t)SHM_KEY,  MEM_SIZE,  IPC_CREAT | IPC_EXCL | 0644);
	if (shm_id < 0){

		if (errno == EEXIST){
			shm_id = shmget((key_t)SHM_KEY, MEM_SIZE, 0 );
			if (shm_id < 0){
				fprintf(stderr," Fail to shmget()");
				return -1;
			}
			
			psmConf.shm_addr = (char *)shmat(shm_id, (void *)0, 0 ) ;
			if (psmConf.shm_addr == (char *) -1){
				fprintf(stderr," Fail to shmat()");
				return -1;
			}
			fprintf(stderr," Shmget[EXIST]");
			return SHM_EXIST;
		}else
			return -1;
	}

	if( (psmConf.shm_addr = (char *)shmat( shm_id, (void *)0, 0 )) == (void *)-1 ){
		fprintf(stderr, "shmat is failed\n");
		return -1;
	}

	memset(psmConf.shm_addr, 0x00, MEM_SIZE);
	fprintf(stderr," Shmget[CREATE]");

	return SHM_CREATE;
}
