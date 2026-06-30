#Makefile Trabalho 1

CC=gcc
CFLAGS=-g -Wall 
ENDFLAGS= -lm -pthread


cpu_stress: cpu_stress.o 
	$(CC) $(CFLAGS) $^ -o $@ $(ENDFLAGS)

vector_threads: vector_threads.o vector_stat_seq.o
	$(CC) $(CFLAGS) $^ -o $@ $(ENDFLAGS)

vector_stat_client: vector_stat_client.o vector_stat_seq.o vector_stat_proc_threads.o vector_stat_proc.o
	$(CC) $(CFLAGS) $^ -o $@ $(ENDFLAGS)
	
vector_stat_server: vector_stat_server.o tcp_connection.o unix_connection.o vector_stat_seq.o vector_stat_proc_threads.o vector_stat_proc.o
	$(CC) $(CFLAGS) $^ -o $@ $(ENDFLAGS)
	

%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f *.o
	rm -f a.out

