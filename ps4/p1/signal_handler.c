#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <getopt.h>
#include <limits.h>
#include <linux/limits.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <ctype.h>
#include <glob.h>
#include <time.h>


int niter;
volatile int count;

void hh(int sig)
{
    count++;
}

void xx(int sig)
{
    fprintf(stderr,"Child got signal %d, count is %d \n",sig,count);
    exit(128);
}

int main(int argc, char **argv){

    struct sigaction sa;
    int i, cpid, ws, signum;
    struct timespec ts;
 
    niter=atoi(argv[1]); // number of times sending signal
    signum=atoi(argv[2]); //signal number

    sa.sa_handler=hh;
    sa.sa_flags = 0;

    sigemptyset(&sa.sa_mask);
    (void)sigaction(signum,&sa,NULL);
    
    signal(SIGINT,xx);
    
    switch (cpid=fork())
    {
    case 0:
        for(;;)
            ;
        fprintf(stderr,"Child broke loop\n");exit(1);
    default:
        for(i=0;i<niter;i++)
            kill(cpid,signum);
        ts.tv_sec=1;ts.tv_nsec=0;
        nanosleep(&ts,0);
        kill(cpid,SIGINT);
    }
    wait(&ws);
    fprintf(stderr,"Parent got wait status %x\n",ws);
    return 0;
}