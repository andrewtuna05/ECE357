#include <stdio.h>
#include "myshell.h"

int main(int argc, char **argv) {
    // Start the shell loop
    // while (getopt(argc, argv, "") != -1) {
    //     switch (opt) {
    //         case '?':
    //         default:
    //             fprintf(stderr, "Usage: %s\n", argv[0]);
    //             exit(EXIT_FAILURE);
    //     }
    // }

    return myshell_loop();

}