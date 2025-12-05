#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>

#include "fifo.h"
#include "sem.h"

#define WID_BITS  32
#define SEQ_MASK  0xffffffffUL  // lower 32 bits set

int my_procnum;

// To prevent our program from just dying 
void sigusr1_handler(int sig){
    (void)sig;
}

// wid (upper 32 bits) | seq (lower 32 bits)
unsigned long make_word(unsigned int wid, unsigned int seq){
    unsigned long high = (unsigned long)wid;
    unsigned long low = (unsigned long)seq;
    unsigned long word = (high << WID_BITS) | low;
    return word;
}

// Extract writer id from word
unsigned int word_wid(unsigned long word){
    return (unsigned int)(word >> WID_BITS);
}

// Extract sequence number from word
unsigned int word_seq(unsigned long word){
    return (unsigned int)(word & SEQ_MASK);
}

void run_fifo_test(int n_writers, int n_per_writer){
    int total_items = n_writers * n_per_writer;

    printf("Beginning test with %d writers, %d items each\n", n_writers, n_per_writer);

    // mmap shared FIFO
    struct myfifo *f = mmap(NULL, sizeof(*f), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    
    if (f == MAP_FAILED){
        perror("mmap");
        exit(1);
    }

    fifo_init(f);

    int next_procnum = 0;

    // Reader process 
    pid_t rpid = fork(); // one reader

    if (rpid < 0){
        perror("fork reader");
        exit(1);
    }

    if (rpid == 0){
        // child is reader
        my_procnum = next_procnum++;

        unsigned long last_seq[n_writers];

        // initialize so first expected seq is 0
        for (int i = 0; i < n_writers; i++){
            last_seq[i] = (unsigned long)-1; // casting for warning issues 
        }

        int completed_streams = 0;

        for (int k = 0; k < total_items; k++){
            unsigned long word = fifo_rd(f);

            unsigned int wid = word_wid(word);
            unsigned int seq = word_seq(word);

            unsigned long expected = last_seq[wid] + 1;

            if (seq != expected){
                fprintf(stderr,"Detected out-of-sequence word %u:%u (expecting %lu)\n", wid, seq, expected);
                _exit(1);
            }

            last_seq[wid] = seq;

            if (seq == (unsigned int)(n_per_writer - 1)){
                printf("Reader stream %u completed\n", wid);
                completed_streams++;
            }
        }

        if (completed_streams == n_writers){
            printf("All streams done\n");
            _exit(0);
        }

        fprintf(stderr, "Not all streams completed\n"); //debugging
       _exit(1); // otherwise somewhere we goofed
    }

    // parent continues
    next_procnum++;


    // writer process
    for (int w = 0; w < n_writers; w++) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork writer");
            exit(1);
        }
        if (pid == 0) {
            // child is writer w
            my_procnum = next_procnum++;   

            for (int seq = 0; seq < n_per_writer; seq++) {
                unsigned long word = make_word((unsigned int)w, (unsigned int)seq);
                fifo_wr(f, word);
            }
            printf("Writer %d completed\n", w);
            _exit(0);
        }
        // parent
        next_procnum++;
    }

    int status;
    int failures = 0;
    int n_children = n_writers + 1; // writers + reader

    for (int i = 0; i < n_children; i++) {
        pid_t pid = wait(&status);
        if (pid < 0) {
            perror("wait");
            exit(1);
        }
    }
    printf("Waiting for writer children to die\n");

    if (failures == 0) {
        printf("All children exited normally\n");
    }else {
        printf("%d children exited abnormally\n", failures); // for debugging
    }
}

int main(int argc, char **argv){
    struct sigaction sa;
    sa.sa_handler = sigusr1_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(SIGUSR1, &sa, NULL) < 0) {
        perror("sigaction");
        exit(1);
    }

    int n_writers = 0;
    int n_per_writer = 0;
    int opt;

    while ((opt = getopt(argc, argv, "w:n:")) != -1){
        switch (opt){
        case 'w':
            n_writers = atoi(optarg);
            break;
        case 'n':
            n_per_writer = atoi(optarg);
            break;
        default:
            fprintf(stderr, "Usage: %s [-w writers] [-n items_per_writer]\n", argv[0]);
            exit(1);
        }
    }

    run_fifo_test(n_writers, n_per_writer);

    return 0;
}