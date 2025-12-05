CFLAGS = -Wall -Wextra -g -D_XOPEN_SOURCE=700
DEBUGFLAGS = -g

all: clean testspin main
main: main.o fifo.o sem.o
	@gcc $(CFLAGS) -o main.out main.o fifo.o sem.o spin_lock.o tas64.o
run: main
	@./main.out
testspin: test_spin_lock.o spin_lock.o tas.o
	@gcc $(CFLAGS) -o test_spin_lock test_spin_lock.o spin_lock.o tas64.o
test_spin_lock.o: test_spin_lock.c spin_lock.h tas.h
	@gcc $(CFLAGS) -c test_spin_lock.c

debug: clean
	@gcc $(CFLAGS) $(DEBUGFLAGS) -c runner.c
	@gcc $(CFLAGS) $(DEBUGFLAGS) -c bgrep.c
	@gcc $(CFLAGS) $(DEBUGFLAGS) -o bgrep runner.o bgrep.o

spin_lock.o: spin_lock.h spin_lock.c tas.o
	@gcc $(CFLAGS) -c spin_lock.c

tas.o: tas.h tas64.S
	@gcc $(CFLAGS) -c tas64.S
clean:
	@rm -rf *.o main.out            
test_sem: test_sem.o sem.o spin_lock.o
	@gcc $(CFLAGS) -o test_sem test_sem.o sem.o spin_lock.o
test_sem.o: test_sem.c sem.h spin_lock.h
	@gcc $(CFLAGS) -c test_sem.c
fifo.o: fifo.h fifo.c sem.o
	@gcc $(CFLAGS) -c fifo.c
main.o: main.c fifo.h sem.h
	@gcc $(CFLAGS) -c main.c
sem.o: sem.h sem.c spin_lock.o
	@gcc $(CFLAGS) -c sem.c     