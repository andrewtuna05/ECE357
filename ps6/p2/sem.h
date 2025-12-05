#ifndef SEM_H
#define SEM_H
#define N_PROC 64

#include "spin_lock.h"
#include <unistd.h>
#include <sys/syscall.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>

struct sem{
    volatile char lock; // spinlock for the semaphore
    int count; // semaphore value for the operation type
    int waiting[N_PROC]; // make a bitmap of waiting processes per your suggestion
    pid_t pids[N_PROC];   // contain the actual PIDs of the waiting processes
};

void sem_init(struct sem *s, int count);

int sem_try(struct sem *s);

void sem_wait(struct sem *s);

void sem_inc(struct sem *s);

extern int my_procnum; 

#endif