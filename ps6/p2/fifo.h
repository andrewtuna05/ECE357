#ifndef FIFO_H
#define FIFO_H
#define N_PROC 64
#define MYFIFO_BUFSIZ 4096

#include "sem.h"

struct myfifo {
    unsigned long buffer[MYFIFO_BUFSIZ];
    int head; // next index to read
    int tail; // next index to write
    
    struct sem empty; // writers wait here when no empty slots
    struct sem full; // readers wait here when no data
    struct sem mutex; // short-lived mutex for issue 1
};

void fifo_init(struct myfifo *f);

void fifo_wr(struct myfifo *f, unsigned long d);

unsigned long fifo_rd(struct myfifo *f);

#endif