#define N_PROC 64

#include "spin_lock.h"
#include "sem.h"
#include <signal.h>

void sem_init(struct sem *s, int count){
    s->lock = 0; // initialize spinlock to unlocked state
    s->count = count;

    for (int i = 0; i < N_PROC; i++){
        s->waiting[i] = 0;  // initialize waiting bitmap to 0 for not blocked
        s->pids[i] = -1;
    }
}

//Perform P operation, blocking until successful - note: avoid lost wakeup

void sem_wait(struct sem *s){
    sigset_t newmask, oldmask;
    sigemptyset(&newmask);
    sigaddset(&newmask, SIGUSR1);
    
    
    while (1){
        // Block SIGUSR1
        sigprocmask(SIG_BLOCK, &newmask, &oldmask);
        
        spin_lock(&s->lock);
        
        if (s->count > 0){
            s->count--;
            spin_unlock(&s->lock);
            // Restore old mask
            sigprocmask(SIG_SETMASK, &oldmask, NULL);
            return;
        }else{
            s->waiting[my_procnum] = 1; // blocked
            s->pids[my_procnum] = getpid(); // Store the PID of the waiting process
            spin_unlock(&s->lock);
            
            // Wait for SIGUSR1
            sigsuspend(&oldmask); // Atomically unblocks SIGUSR1
        
            // // Now sleep with SIGUSR1 unmasked. MAYBE THIS MIGHT WORK
            // sigset_t suspendmask = oldmask;
            // sigdelset(&suspendmask, SIGUSR1);

            // sigsuspend(&suspendmask);

            sigprocmask(SIG_SETMASK, &oldmask, NULL);
        }
    }
}

// Perform V operation
void sem_inc(struct sem *s){
    spin_lock(&s->lock);
    s->count++;

    // wake up all sleeping tasks. if there are multiple sleepers then all must be sent the wakeup
        for(int i = 0; i < N_PROC; i++){
            if(s->waiting[i] == 1){
                kill(s->pids[i], SIGUSR1); // send signal to wake up
                s->waiting[i] = 0; // mark as not blocked
            }
        }

    spin_unlock(&s->lock);
}

// If operation would block, return 0, otherwise return 1 and decrement
int sem_try(struct sem *s){
        spin_lock(&s->lock);

    if (s->count > 0){
        s->count--;
        spin_unlock(&s->lock);
        return 1;
    }
 
    spin_unlock(&s->lock);
    return 0;
}
