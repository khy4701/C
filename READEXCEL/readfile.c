#include <stdio.h>
#include <unistd.h>
#include <errno.h>

#define MAX_BUFF_SIZE 1000
extern int errno;

int main()
{
    FILE *fp , *write_fp;
    int data, i, p;

    fp = fopen("test.xlsx", "r");
    if( fp == NULL)
    {
        printf("FILE READ ERROR : %d\n", errno);
    }

    while( fscanf(fp, "%d", &data) != EOF ){

        printf("%d",data);

    }

    fclose(fp);

}
