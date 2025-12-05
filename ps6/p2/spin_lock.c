#include "spin_lock.h"
#include "tas.h"

void spin_lock(volatile char *lock){

    while (tas(lock) != 0){
        sched_yield();
    }
    return;
}

void spin_unlock(volatile char *lock){
    *lock = 0;
    return;
}