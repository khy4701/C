#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

void sig_interrupt_handler(int signo);
void sig_stop_handler(int signo);

// SIG STOP(CTRL+Z) 받을 경우
// 1. Parent Process --> Child Process 생성
// 2. Child Process Number[5] Count , Parent Process Waiting until child process down
// 3. Child Processing main job and die
// 4. Parent Process restart and process main job


int main(){

    int i=0;
    signal(SIGTSTP, (void *)sig_stop_handler);
    signal(SIGINT, (void *)sig_interrupt_handler);

    while(1){
        printf("%d\n",i );
        sleep(1);
        i++;
		if( i== 10)
			break;
    }

}

void sig_interrupt_handler(int signo){

    printf(" Received SIGINT(%d)\n", SIGINT);
    exit(1);

}

void sig_stop_handler(int signo){

	int pid = 0, ret;
	int i =0;
	int status;

    printf(" Received SIGSTOP(%d) , sleepCnt: 3(sec)\n", SIGSTOP);

    pid = fork();

	// CHILD
	if (pid == 0){
	printf("Child Process is Running...\n");
		while(1){
			if( i++ >= 5 )
				break;
			printf("Process Child is Process [%d]\n", i);
			sleep(1);
		}
		printf("Child Process is ready to die\n");
		return ;
	}

	// PARETN
	printf("Parent Process is waiting...\n");

	// 자식 프로세스가 죽을때 까지 기다림( WIFEXITED )
	ret = waitpid(pid, &status, WIFEXITED(&status));
	if (ret < 0){
		printf("Error to waitpid\n");
		exit(1);
	}

	printf("Parent Process is ready to die\n");

	return ;

}

