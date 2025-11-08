#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <time.h>
#include <stdbool.h>
#include <ctype.h>

int main(int argc, char** argv){

    FILE* tty = fopen("/dev/tty", "r");

    char buffer[256];
    int line_count = 0;

    while (fgets(buffer, sizeof(buffer), stdin)) {
        fputs(buffer, stdout);  // Display line immediately
        line_count++;

        if(line_count == 23){
             printf("---Press RETURN for more---\n");
            int c = fgetc(tty);
            if (c == 'q' || c == 'Q' || c == EOF){      // Ending condition
                if (c == 'q' || c == 'Q'){
                    printf("*** Pager terminated by Q Command ***\n");
                }
                break;
            }
            line_count = 0; // Reset and dispaly more lines
        }

    }
    fclose(tty);
    return 0;
}

