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
#include <time.h>

static sigjmp_buf env;
char *current_file = NULL;

void signal_handler(int sig) {
    write(STDERR_FILENO, "SIGBUS received while processing file ", 39);
    write(STDERR_FILENO, current_file, strlen(current_file));
    write(STDERR_FILENO, "\n", 1);
    siglongjmp(env, 1);
}

void scan_file(char *filename, char *pattern, unsigned long len, int context, bool *sys_error, bool *global_match){
    int fd;
    struct stat st;

    // If stdin then report the error
    if (filename == NULL){
        fprintf(stderr, "stdin cannot be mmapped\n");
        *sys_error = true;
        return;
    }

    fd = open(filename, O_RDONLY);
    if (fd < 0){
        fprintf(stderr, "Can't open %s for reading: %s\n", filename, strerror(errno));
        *sys_error = true;
        return;
    }

    if (fstat(fd, &st) < 0){
        fprintf(stderr, "fstat failed on %s: %s\n", filename, strerror(errno));
        *sys_error = true;
        close(fd);
        return;
    }

    // If the file is empty
    if (st.st_size < len){
        close(fd);
        return;
    }

    //stackoverflow.com/questions/13642381/c-c-why-to-use-unsigned-char-for-binary-data?
    unsigned char *addr = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (addr == MAP_FAILED){
        fprintf(stderr, "mmap failed on %s: %s", filename, strerror(errno));
        *sys_error = true;
        close(fd);
        return;
    }

    // SIGBUS https://stackoverflow.com/questions/20755260/why-use-sigsetjmp-instead-of-setjmp-function-in-c
    current_file = filename;

    // If SIGBUS occurred, skip and close out the file
    if (sigsetjmp(env, 1) != 0) {
        munmap(addr, st.st_size);
        close(fd);
        *sys_error = true;
        return;
    }

    // Pattern searching through file
    for (off_t i = 0; i + len <= st.st_size; i++){
        if (memcmp(addr + i, pattern, len) == 0){
           *global_match = true;

            if (context == 0){
                // print filename + offset
                printf("%s:%ld\n", filename, (long)i); // cast into long insead of off_t (refer to previous source down below)

            }else{
                // compute context range
                size_t start;
                if (i >= context){
                    start = i - context;
                }else{
                    start = 0;
                }

                size_t end = i + len + context;
                if (end > st.st_size){
                    end = st.st_size;
                }

                printf("%s:%ld ", filename, (long)i); // cast into long insead of off_t (refer to previous source down below)

                // ASCII
                for (size_t j = start; j < end; j++){
                    unsigned char c = addr[j];
                    if(isprint(c)){
                        putchar(c);
                        putchar(' ');
                    }
                    else{
                        putchar('?');
                        putchar(' ');
                    }
                }
                putchar('\t');

                // HEX
                for (size_t j = start; j < end; j++){
                    printf("%02x ", addr[j]);
                }
                putchar('\n');
            }   
        }
    }

    munmap(addr, st.st_size);
    close(fd);
}

int main(int argc, char** argv){

    int context = 0;
    bool use_pattern = false;
    char *pattern_file = NULL;

    //command parsing
    int opt;

    if (argc == 1) {
        fprintf(stderr, "Error: bgrep arguments not provided\n");
        return 1;
    }

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    sigaction(SIGBUS, &sa, NULL);

    while ((opt = getopt(argc, argv, "pc:")) != -1) {
        switch (opt) {
            case 'c': 
                context = atoi(optarg); // set amount of context bytes
                break;
            case 'p': 

                if (optind >= argc) { // requires file input if -p flag
                    fprintf(stderr, "Error: -p requires a pattern file\n"); 
                    return 1;
                }

                use_pattern = true;
                pattern_file = argv[optind]; // set pattern_file
                optind++;   // consume the pattern filename
                break;
                
            default:
                fprintf(stderr, "usage: bgrep [-c N] [-p patternfile] pattern files...\n"); // proper usage of command
                return 1;
        }
    }

    if (!use_pattern && optind >= argc) { // if no pattern was supplied
        fprintf(stderr, "Error: no search pattern provided\n");
        return 1;
    }

    char *pattern = NULL;
    size_t len = 0; //www.reddit.com/r/C_Programming/comments/e4hro6/when_to_use_size_t/

    // Pattern comes from -p or literal pattern
    // Case 1: -p provided
    if(use_pattern == true){
        int p_fd = open(pattern_file, O_RDONLY);
        if (p_fd < 0) {
            fprintf(stderr, "Can't open pattern file %s: %s\n", pattern_file, strerror(errno));
            return -1;
        }

        struct stat p_st;
        if (fstat(p_fd, &p_st) < 0) {
            fprintf(stderr, "fstat failed on %s: %s\n", pattern_file, strerror(errno));
            close(p_fd);
            return -1;
        }

        len = p_st.st_size;
        if (len == 0){
            fprintf(stderr, "Error: pattern file %s is empty\n", pattern_file);
            close(p_fd);
            return 1;
        }
        
        pattern = malloc(len);
        read(p_fd, pattern, len);
        close(p_fd);

    // Case 2: Literal Pattern
    }else{
        pattern = argv[optind];
        len = strlen((char*)pattern);
        optind++;  // consume literal pattern
    }
    
    bool sys_error = false;
    bool global_match = false;
    bool use_stdin = false;

    if (optind >= argc){
        use_stdin = true;
    }

    if (use_stdin){
        scan_file(NULL, pattern, len, context, &sys_error, &global_match);
    } else{
        for (int i = optind; i < argc; i++){
            scan_file(argv[i], pattern, len, context, &sys_error, &global_match);
        }
    }

    if (sys_error){
        return -1;
    }

    if (global_match){
        return 0;
    }
    return 1;
}