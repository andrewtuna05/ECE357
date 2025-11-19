CC = gcc
CFLAGS = -Wall -Wextra

PROGS = bgrep

all: $(PROGS)

bgrep: bgrep.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(PROGS) *.o
