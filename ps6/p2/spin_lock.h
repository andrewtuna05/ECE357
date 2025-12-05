#ifndef SPINLOCK_H
#define SPINLOCK_H
#define N_PROC 64

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <signal.h>
#include <errno.h>
#include <sched.h>
#include <time.h>
#include <stdbool.h>
#include "tas.h"


void spin_lock(volatile char *lock);

void spin_unlock(volatile char *lock);

#endif