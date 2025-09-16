#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int main(void)
{	
	int n = write(0, "41CS", 4);
	printf("%d\n", errno);
	perror("write");
}
