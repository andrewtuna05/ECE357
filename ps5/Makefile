CC = gcc
CFLAGS = -Wall -Wextra -g -D_XOPEN_SOURCE=700

PROGS = bgrep

all: $(PROGS)

bgrep: bgrep.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(PROGS) *.o
