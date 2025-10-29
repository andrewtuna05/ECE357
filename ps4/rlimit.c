#include <stdio.h>
#include <stdlib.h>
#include <sys/resource.h>

int main(void) {
    struct rlimit rlim;

    printf("RLIMIT_SIGPENDING Soft limit: %ld\n", (long) rlim.rlim_cur);
    printf("RLIMIT_SIGPENDING Hard limit: %ld\n", (long) rlim.rlim_max);

    return 0;
}