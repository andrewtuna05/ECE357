#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <time.h>
#include <ctype.h>

int main(int argc, char** argv){

    // Signal handler - Ignore the SIGPIPE so that I can manually fix it
    signal(SIGPIPE, SIG_IGN);  


    // Parse argument for file
    if (argc != 2) {
        fprintf(stderr, "wordsearch: invalid argument\n");
        return 1;
    }

    FILE *dict = fopen(argv[1], "r");
    if (!dict) {
        fprintf(stderr, "wordsearch: could not open file '%s'\n", argv[1]);
        return 1;
    }

    char *wordlist[500000]; // Github desc. says word file is ~500K
    char buffer[256]; // Some arbitrary size
    int dict_count = 0;
    
    // Load dictionary
    while (fgets(buffer, sizeof(buffer), dict)) {
        buffer[strcspn(buffer, "\n")] = '\0'; // Remove newline 
        
        // I originally had the reject non-english letters code but then i realized theres a word_alpha.txt so i decided to just use that instead
        //https://github.com/dwyl/english-words/tree/master

        // In fact I found another smaller source  https://users.cs.duke.edu/~ola/ap/linuxwords that just contains valid words so no more rejected

        // Convert to uppercase
        for (char *p = buffer; *p; p++)
            *p = toupper((unsigned char)*p);

        wordlist[dict_count] = strdup(buffer);
        dict_count++;
    }
    fclose(dict);
    fprintf(stderr, "Accepted %d words\n", dict_count);
    
    // Process stdin
    char input[256];
    int matched = 0;

    while (fgets(input, sizeof(input), stdin)) {
        input[strcspn(input, "\n")] = '\0';
        for (char *p = input; *p; p++)
            *p = toupper((unsigned char)*p);

        int r = 0;
        for (int i = 0; i < dict_count; i++) {
            if (strcmp(input, wordlist[i]) == 0) {
                r = printf("%s\n", input);

                if (r < 0 && errno == EPIPE) { // Manually catch the EPIPE Error
                    break;
                }
                matched++;
                break;
            }
        }
        
        if (r < 0 && errno == EPIPE) { // Manually catch the EPIPE Error
            break;
        }
    }
    fprintf(stderr, "Matched %d words\n", matched);
    return 0;
}