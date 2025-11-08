#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>
#include <getopt.h>
#include <ctype.h>

int main(int argc, char *argv[]) {
    int limit = 0;
    if (argc == 2) {
        limit = atoi(argv[1]);
        if (limit < 0) {
            fprintf(stderr, "wordgen: invalid argument '%s'\n", argv[1]);
            return 1;
        }
    }

    int p1[2]; // wordgen to wordsearch 0 = Read, 1 = Write based on pipe()
    int p2[2]; // wordsearch to pager

    if (pipe(p1) == -1) {
        perror("Pipe p1");
        exit(1);
    }
    if (pipe(p2) == -1) {
        perror("Pipe p2");
        exit(1);
    }

    pid_t pid1 = fork(); //wordgen
    if (pid1 == 0) {
        dup2(p1[1], STDOUT_FILENO); // Put wordgen output to the pipe to wordsearch
        close(p1[0]);
        close(p1[1]);                     
        close(p2[0]);    
        close(p2[1]);      

        if (limit > 0) {
            char limit_str[12]; // Some arbitrary size 
            sprintf(limit_str, "%d", limit); //Convert argv[1] to str
            execl("./wordgen", "./wordgen", limit_str, NULL);
        } else {
            execl("./wordgen", "./wordgen", NULL);
        }
        fprintf(stderr, "launcher: exec wordgen failed: %s\n", strerror(errno));
        _exit(127); // 127 = exec failed
    }

    pid_t pid2 = fork(); // wordsearch
    if (pid2 == 0){
        dup2(p1[0], STDIN_FILENO); // Output of wordgen goes into wordsearch
        close(p1[0]);
        close(p1[1]);
        dup2(p2[1], STDOUT_FILENO); // Output of wordsearch into pager
        close(p2[1]);
        close(p2[0]);
        execl("./wordsearch", "./wordsearch", "words.txt", NULL);
        fprintf(stderr, "launcher: exec wordsearch failed: %s\n", strerror(errno));
        _exit(127); //127 = exec failed
    }

    pid_t pid3 = fork();
    if (pid3 == 0){
        dup2(p2[0], STDIN_FILENO);
        close(p2[0]);
        close(p2[1]);
        close(p1[0]);
        close(p1[1]);

        execl("./pager", "./pager", NULL);
        fprintf(stderr, "launcher: exec pager failed: %s\n", strerror(errno));
        _exit(127); //127 = exec failed
    }

    // Back in the parent --> close all copies
    close(p1[0]); 
    close(p1[1]);
    close(p2[0]); 
    close(p2[1]);

    // Wait for all children
    int status1;
    int status2;
    int status3;

    // Wordgen
    waitpid(pid1, &status1, 0);
    if (WIFEXITED(status1))
        fprintf(stderr, "Child %d exited with %d\n", pid1, WEXITSTATUS(status1));
    else if (WIFSIGNALED(status1))
        fprintf(stderr, "Child %d exited with %d\n", pid1, WTERMSIG(status1));

    // Wordsearch
    waitpid(pid2, &status2, 0);
    if (WIFEXITED(status2))
        fprintf(stderr, "Child %d exited with %d\n", pid2, WEXITSTATUS(status2));
    else if (WIFSIGNALED(status2))
        fprintf(stderr, "Child %d exited with %d\n", pid2, WTERMSIG(status2));

    // Pager
    waitpid(pid3, &status3, 0);
    if (WIFEXITED(status3))
        fprintf(stderr, "Child %d exited with %d\n", pid3, WEXITSTATUS(status3));
    else if (WIFSIGNALED(status3))
        fprintf(stderr, "Child %d exited with %d\n", pid3, WTERMSIG(status3));

    return 0;
}


