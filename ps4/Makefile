CC = gcc
CFLAGS = -Wall -Wextra

PROGS = wordgen wordsearch pager launcher

all: $(PROGS)

wordgen: wordgen.c
	$(CC) $(CFLAGS) -o $@ $<

wordsearch: wordsearch.c
	$(CC) $(CFLAGS) -o $@ $<

pager: pager.c
	$(CC) $(CFLAGS) -o $@ $<

launcher: launcher.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(PROGS) *.o
