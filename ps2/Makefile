CC = gcc
CFLAGS = -Wall -Wextra -g -D_XOPEN_SOURCE=700

simplefind: simplefind.o
	$(CC) $(CFLAGS) -o simplefind simplefind.o

simplefind.o: simplefind.c
	$(CC) $(CFLAGS) -c simplefind.c

clean:
	rm -f simplefind simplefind.o