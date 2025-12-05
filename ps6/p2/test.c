#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdint.h>

#include "spin_lock.h"   // your spin_lock/spin_unlock
#include "tas.h"        // provided TAS

#define N_PROCS 10        // or use nproc()
#define N_ITERS 2000000   // 2 million increments per process

/*
 * Shared memory structure.
 * Both the lock and the counter must live in mmap() SHARED memory.
 */
struct shared {
    volatile int lock;
    int counter;
};

int main() {

    // Allocate shared memory
    struct shared *sh = mmap(NULL, sizeof(struct shared),
                             PROT_READ | PROT_WRITE,
                             MAP_SHARED | MAP_ANONYMOUS,
                             -1, 0);

    if (sh == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }

    printf("=== Test 1: UNPROTECTED increments ===\n");

    sh->counter = 0;

    // Spawn processes
    for (int p = 0; p < N_PROCS; p++) {
        pid_t pid = fork();

        if (pid < 0) {
            perror("fork");
            exit(1);
        }

        if (pid == 0) {
            // Child increments counter WITHOUT ANY LOCK
            for (int i = 0; i < N_ITERS; i++) {
                sh->counter++;   // race condition!
            }
            exit(0);
        }
    }

    // Parent waits
    for (int p = 0; p < N_PROCS; p++)
        wait(NULL);

    long expected = N_PROCS * N_ITERS;
    printf("Unprotected result = %d (expected %ld)\n\n", sh->counter, expected);


    printf("=== Test 2: PROTECTED increments (spinlock) ===\n");

    sh->counter = 0;
    sh->lock = 0;       // unlocked

    for (int p = 0; p < N_PROCS; p++) {
        pid_t pid = fork();

        if (pid < 0) {
            perror("fork");
            exit(1);
        }

        if (pid == 0) {
            // Child increments WITH proper TAS spinlock
            for (int i = 0; i < N_ITERS; i++) {
                spin_lock(&sh->lock);
                sh->counter++;
                spin_unlock(&sh->lock);
            }
            exit(0);
        }
    }

    // Parent waits
    for (int p = 0; p < N_PROCS; p++)
        wait(NULL);

    printf("Protected result = %d (expected %ld)\n", sh->counter, expected);
    printf("If spinlock is correct, these should match.\n");

    return 0;
}
