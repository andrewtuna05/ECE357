#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include<string.h>

int main(void) {
    write(0, "41CS", 4);
    FILE *fp = fdopen(0, "r");

    return 0;
}
