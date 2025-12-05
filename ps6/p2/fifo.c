#include <unistd.h>
#include <sys/syscall.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include "fifo.h"
#include <sys/mman.h>

void fifo_init(struct myfifo *f){
    f->head = 0;
    f->tail = 0;

    //FIFO is initially all empty
    sem_init(&f->empty, MYFIFO_BUFSIZ);
    sem_init(&f->full, 0);

    // mutex used as a spin-lock: count 1 = not blocked
    sem_init(&f->mutex, 1);
}

void fifo_wr(struct myfifo *f, unsigned long d){
    sem_wait(&f->empty); //wait for at least 1 empty slot

    while(sem_try(&f->mutex) == 0); //spin until we get the mutex

    //we're in the endgame now (critical region)
    f->buffer[f->tail] = d;
    f->tail = (f->tail + 1) % MYFIFO_BUFSIZ; // round it back

    // release the spin lock
    sem_inc(&f->mutex);

    sem_inc(&f->full);
}

unsigned long fifo_rd(struct myfifo *f){
    unsigned long d;

    sem_wait(&f->full); // wait for at least one item since you cant read nothing (clearly hasnt met the students of ECE357)

    while (sem_try(&f->mutex) == 0); //spin until we get the mutex

    //we're in the endgame now (crtical region)
    d = f->buffer[f->head];
    f->head = (f->head + 1) % MYFIFO_BUFSIZ; // round it back

    // release the spin lock
    sem_inc(&f->mutex);

    sem_inc(&f->empty);

    return d;
}