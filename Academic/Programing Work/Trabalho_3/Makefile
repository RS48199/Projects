#Makefile Trabalho 3

CC=gcc
CFLAGS=-g -Wall 
ENDFLAGS= -lm -pthread
	
ex1a: ex1a.o
	$(CC) $(CFLAGS) $^ -o $@ $(ENDFLAGS)
	
ex1b: ex1b.o
	$(CC) $(CFLAGS) $^ -o $@ $(ENDFLAGS)
	
ex2a: ex2a.o
	$(CC) $(CFLAGS) $^ -o $@ $(ENDFLAGS)

ex3: ex3.o
	$(CC) $(CFLAGS) $^ -o $@ $(ENDFLAGS)
	
thread_pool: tptest.o thp.o
	$(CC) $(CFLAGS) $^ -o $@ $(ENDFLAGS)
	
vector_thread_pool: ex3.o thp.o vector_tp.o
	$(CC) $(CFLAGS) $^ -o $@ $(ENDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f *.o
	rm -f a.out

