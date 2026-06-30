#include <stdio.h>
#include <pthread.h>


void *th1 (void *arg) 
{
	int *pt = (int *)arg;
	for (int i=0 ; i<10000000; ++i)
		(*pt) += 2;
	
	printf("Th1: %d\n", *pt);
	
	return NULL;
}

void *th2 (void *arg) 
{
	int *pt = (int *)arg;
	for (int i=0 ; i<10000000; ++i)
		(*pt) -= 3;
	
	printf("Th2: %d\n", *pt);
		
	return NULL;
}

void *th3 (void *arg) 
{
	int *pt = (int *)arg;
	for (int i=0 ; i<10000000; ++i)
		(*pt)++;
	
	printf("Th3: %d\n", *pt);
		
	return NULL;
}

int main() 
{
	int count = 0;
	
	int arguments[3];
	for(int i=0; i<3; i++)
		arguments[i] = count;
	
	pthread_t t1, t2, t3;
	pthread_create(&t1, NULL, th1, &arguments[0]);
	pthread_create(&t2, NULL, th2, &arguments[1]);
	pthread_create(&t3, NULL, th3, &arguments[2]);
	pthread_join(t1, NULL);
	pthread_join(t2, NULL);
	pthread_join(t3, NULL);
	
	printf("---Results---\n");
	for(int i=0; i<3; i++)
	{
		count = count + arguments[i];
		printf("Argument %d, value %d\n", i, arguments[i]);
	}	
	printf("Total = %d\n", count);
}

