// test_sem.c
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <signal.h>
#include <assert.h>

#include "sem.h"     // your header
int my_procnum;

// tune these as you like
#define N_TRYERS 16
#define N_PRODUCERS     3
#define N_CONSUMERS     5
#define LOOPS_PER_CONS  10000
#define TOTAL_UNITS     (N_CONSUMERS * LOOPS_PER_CONS)

extern int my_procnum;   // declared in sem.h, defined in your sem.c

struct shared_state {
    struct sem s;
    volatile int consumed;
    int try_results[N_TRYERS];   // new: per-process sem_try result
};

static struct shared_state *SH;

// simple macro for test failures
#define FAIL(msg) do { \
    fprintf(stderr, "TEST FAILED: %s (%s:%d)\n", msg, __FILE__, __LINE__); \
    exit(1); \
} while (0)

// Optional: a no-op handler in case your library expects SIGUSR1 to be catchable.
// If your semaphore code installs its own handler, you can delete this.
static void sigusr1_handler(int sig) {
    (void)sig;
}

static void setup_signals(void) {
    struct sigaction sa;
    sa.sa_handler = sigusr1_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGUSR1, &sa, NULL) < 0) {
        perror("sigaction");
        exit(1);
    }
}

int main(void)
{
    // 1. Shared memory
    SH = mmap(NULL, sizeof(*SH),
              PROT_READ | PROT_WRITE,
              MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (SH == MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    setup_signals();

    // 2. Initialize semaphore and counter
    sem_init(&SH->s, 0);    // start at 0 so consumers must block
    SH->consumed = 0;

    printf("Testing semaphore with %d producers, %d consumers, %d loops each\n",
           N_PRODUCERS, N_CONSUMERS, LOOPS_PER_CONS);

    int next_procnum = 0;

    // 3. Fork consumers
    for (int c = 0; c < N_CONSUMERS; c++) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork consumer");
            return 1;
        }
        if (pid == 0) {
            // child: consumer
            my_procnum = next_procnum++;        // unique index in [0, N_PROC)
            if (my_procnum >= N_PROC) {
                fprintf(stderr, "Need N_PROC >= %d\n", my_procnum + 1);
                _exit(1);
            }

            for (int i = 0; i < LOOPS_PER_CONS; i++) {
                sem_wait(&SH->s);

                // critical section: increment shared counter
                int new_val = __sync_add_and_fetch(&SH->consumed, 1);

                if (new_val > TOTAL_UNITS) {
                    fprintf(stderr,
                            "[consumer %d] ERROR: consumed=%d > TOTAL=%d\n",
                            my_procnum, new_val, TOTAL_UNITS);
                    _exit(1);
                }
            }
            _exit(0);
        }
        next_procnum++;
    }

    // 4. Fork producers
    int loops_per_prod = TOTAL_UNITS / N_PRODUCERS;
    int extra = TOTAL_UNITS % N_PRODUCERS; // if it doesn't divide evenly

    for (int p = 0; p < N_PRODUCERS; p++) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork producer");
            return 1;
        }
        if (pid == 0) {
            my_procnum = next_procnum++;     // producers can have their own ids too
            if (my_procnum >= N_PROC) {
                fprintf(stderr, "Need N_PROC >= %d\n", my_procnum + 1);
                _exit(1);
            }

            int my_loops = loops_per_prod + (p < extra ? 1 : 0);

            for (int i = 0; i < my_loops; i++) {
                sem_inc(&SH->s);
            }
            _exit(0);
        }
        next_procnum++;
    }

    // 5. Wait for all children to finish
    int status;
    int n_children = N_PRODUCERS + N_CONSUMERS;
    for (int i = 0; i < n_children; i++) {
        pid_t w = wait(&status);
        if (w < 0) {
            perror("wait");
            return 1;
        }
        if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
            fprintf(stderr, "Child %d exited with status %d\n",
                    (int)w, WEXITSTATUS(status));
            FAIL("child failure");
        }
        if (WIFSIGNALED(status)) {
            fprintf(stderr, "Child %d killed by signal %d\n",
                    (int)w, WTERMSIG(status));
            FAIL("child killed");
        }
    }

    // 6. Check final results
    printf("All children finished.\n");
    printf("Expected consumed: %d\n", TOTAL_UNITS);
    printf("Actual consumed  : %d\n", SH->consumed);
    printf("Final sem.count  : %d\n", SH->s.count);

    if (SH->consumed != TOTAL_UNITS) {
        FAIL("wrong final consumed count");
    }
    if (SH->s.count != 0) {
        FAIL("sem.count not back to zero");
    }

    printf("BASIC PRODUCER/CONSUMER TEST PASSED ✅\n");

    // 7. Quick sem_try sanity check on a fresh semaphore
    sem_init(&SH->s, 1);
    int r1 = sem_try(&SH->s);
    int r2 = sem_try(&SH->s);

    printf("sem_try results: first=%d second=%d (expected: first success, second fail)\n",
           r1, r2);

    // depending on your spec, adjust these checks:
    // assume: success => 0, failure => -1
    // For this implementation: sem_try returns 1 on success, 0 on failure
    if (!(r1 == 1 && r2 == 0)) {
        FAIL("sem_try did not behave as expected");
    }

    printf("sem_try TEST PASSED ✅\n");
    // ====================================================
    // Extra stress test: many processes competing for count=1
    // ====================================================
    printf("\nRunning sem_try contention test with %d contenders...\n", N_TRYERS);

    // reset semaphore to 1; only one process should succeed in grabbing it
    sem_init(&SH->s, 1);

    // mark all results as -1 initially
    for (int i = 0; i < N_TRYERS; i++) {
        SH->try_results[i] = -1;
    }

    // fork N_TRYERS children, each calling sem_try exactly once
    for (int i = 0; i < N_TRYERS; i++) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork tryer");
            exit(1);
        }
        if (pid == 0) {
            // child
            my_procnum = i;  // unique index; must be < N_PROC
            int r = sem_try(&SH->s);
            SH->try_results[i] = r;   // 1 = success, 0 = fail in your impl
            _exit(0);
        }
    }

    // wait for all tryer children
    for (int i = 0; i < N_TRYERS; i++) {
        if (wait(&status) < 0) {
            perror("wait tryer");
            exit(1);
        }
        if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
            FAIL("tryer child failure");
        }
        if (WIFSIGNALED(status)) {
            FAIL("tryer child killed by signal");
        }
    }

    // count successes/failures
    int successes = 0, failures = 0;
    for (int i = 0; i < N_TRYERS; i++) {
        if (SH->try_results[i] != 0)  // in your code: non-zero = success
            successes++;
        else
            failures++;
    }

    printf("sem_try contention: successes=%d failures=%d "
           "(expected 1 success, %d failures)\n",
           successes, failures, N_TRYERS - 1);

    if (!(successes == 1 && failures == N_TRYERS - 1)) {
        FAIL("sem_try contention test mismatch");
    }

    // starting from 1, exactly one successful sem_try should decrement to 0
    if (SH->s.count != 0) {
        FAIL("sem_try contention: final sem.count is not 0");
    }

    printf("sem_try contention TEST PASSED ✅\n");
    printf("ALL TESTS PASSED 🎉\n");
    return 0;
}

