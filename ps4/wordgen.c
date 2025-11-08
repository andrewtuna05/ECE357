#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <stdbool.h>
#include <time.h>

int main(int argc, char** argv){
    int limit = 0;
    int count = 0;

    // Parse argument for number of words
    if (argc == 2) {
        limit = atol(argv[1]);
        if (limit < 0) {
            fprintf(stderr, "wordgen: invalid argument '%s'\n", argv[1]);
            return 1;
        }
    }

    srand(time(NULL));

    for(int i = 0; limit == 0 || i < limit; i++){
        int len = (rand() % 8) + 3; // len 3-10
        for (int j = 0; j < len; j++) {
            int ch = 'A' + (rand() % 26);
            putchar(ch);
        }
        putchar('\n');
        count++;
    }

    fprintf(stderr, "Finished generating %d candidate words\n", count);
    fflush(stderr);
    return 0;
}