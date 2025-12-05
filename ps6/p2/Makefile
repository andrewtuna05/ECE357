CC     = gcc
CFLAGS = -Wall -Wextra -g

# Modules
SPIN_OBJS = spin_lock.o tas64.o
SEM_OBJS  = sem.o $(SPIN_OBJS)
FIFO_OBJS = fifo.o $(SEM_OBJS)

# Default: build final FIFO tester and sem tester
all: main test_sem

# Final FIFO test program (Problem 2D) from main.c
main: main.o $(FIFO_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# Semaphore test program from test_sem.c
test_sem: test_sem.o $(SEM_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# Object rules
spin_lock.o: spin_lock.c spin_lock.h tas.h
	$(CC) $(CFLAGS) -c spin_lock.c

tas64.o: tas64.S tas.h
	$(CC) $(CFLAGS) -c tas64.S

sem.o: sem.c sem.h spin_lock.h
	$(CC) $(CFLAGS) -c sem.c

fifo.o: fifo.c fifo.h sem.h
	$(CC) $(CFLAGS) -c fifo.c

main.o: main.c fifo.h sem.h
	$(CC) $(CFLAGS) -c main.c

test_sem.o: test_sem.c sem.h
	$(CC) $(CFLAGS) -c test_sem.c

test.o: test.c fifo.h sem.h
	$(CC) $(CFLAGS) -c test.c

clean:
	rm -f *.o main test_sem fifo_test

.PHONY: all clean
