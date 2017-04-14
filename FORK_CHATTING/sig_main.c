#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>

#define QUEUE_KEY   1000

void handle_parentProcess();
void handle_childProcess();

int main(){

    int     i = 0;
	int     pid = 0, ret;

    pid = fork();
    if (pid == 0){
        printf("Child Process is Running...\n");
        handle_childProcess();
        return ;
    }
    handle_parentProcess();
}

void handle_parentProcess(){
        
}

void handle_childProcess(){

    int queue_id;


}

{
    // 자식 프로세스가 죽을때 까지 기다림( WIFEXITED )
    ret = waitpid(pid, &status, WIFEXITED(&status));
    if (ret < 0){
        printf("Error to waitpid\n");
        exit(1);
    }
