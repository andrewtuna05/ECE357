#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <signal.h>
#include <setjmp.h>
#include <errno.h>
#include <stdbool.h>


int main(int argc, char** argv){

    int context = 0;
    bool use_pattern = false;
    char *pattern_file = NULL;

    //command parsing
    int opt;

    if (argc == 1) {
        fprintf(stderr, "Error: bgrep arguments not provided\n");
        return 255;
    }

    while ((opt = getopt(argc, argv, "pc:")) != -1) {
        switch (opt) {
            case 'c': 
                context = atoi(optarg); // set amount of context bytes
                break;
            case 'p': 

                if (optind >= argc) { // requires file input if -p flag
                    fprintf(stderr, "Error: -p requires a pattern file\n"); 
                    return 255;
                }

                use_pattern = true;
                pattern_file = argv[optind]; // set pattern_file
                optind++;   // consume the pattern filename
                break;
            default:
                fprintf(stderr, "usage: bgrep [-c N] [-p patternfile] pattern files...\n"); // proper usage of command
                return 255;
        }
    }

    if (!use_pattern && optind >= argc) { // if no pattern was supplied
        fprintf(stderr, "Error: no search pattern provided\n");
        return 255;
    }
}