
#include <semaphore.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>




#define MAX_CHARGEPOINT 10
#define MAX_ROBOT 50

typedef struct {
    int position[MAX_CHARGEPOINT];         // Identificador de ponto de carregamento
    long int id[MAX_CHARGEPOINT];          // Informação da thread no respetivo identificador
    int nrSpots;
    int count;
    int waitingThreads;                    // Contador de threads em espera
    int priorityWait;                      // Contador de threads de prioridade em espera
    pthread_mutex_t lock;
    pthread_cond_t wait;
    pthread_cond_t priorityWaitCond;       // Condition variable para threads de prioridade
} Chargepoint;






// Reserva ponto de carregamento e devolve identificador do ponto de carga
int reserveChargePoint(Chargepoint *chargepoint)
{
    // Bloqueio do acesso ao recurso crítico
    pthread_mutex_lock(&(chargepoint->lock));
    int myPosition = -1;
    chargepoint->waitingThreads++;

    do {
        // Verificação de uma posição de carregamento livre
        for (int i = 0; i < MAX_CHARGEPOINT; i++) 
        {
            if (chargepoint->position[i] == -1) 
            {
                myPosition = i;
                break;
            }
        }

        // Se uma posição livre for encontrada
        if (myPosition != -1) 
        {
            // Se houver threads prioritárias espera
            if (chargepoint->priorityWait > 0) 
            {
                pthread_cond_wait(&(chargepoint->priorityWaitCond), &(chargepoint->lock));
                myPosition = -1; // Reset à posição
            }else 
            {
                break;
            }
        } else {
            // Espera passiva das threads normais
            pthread_cond_wait(&(chargepoint->wait), &(chargepoint->lock));
        }

    } while (1);

    chargepoint->waitingThreads--;

    // Atualiza a posição de carregamento
    chargepoint->position[myPosition] = myPosition;
    chargepoint->id[myPosition] = pthread_self();

    printf("\nAcesso a recurso crítico thread nº:%d com ID:%ld\n", chargepoint->count++, pthread_self());
    for (int i = 0; i < MAX_CHARGEPOINT; i++) {
        printf("\t %d \t %ld\n", chargepoint->position[i], chargepoint->id[i]);
    }

    // Recursos críticos
    chargepoint->nrSpots++;

    // Desbloqueio do acesso ao recurso crítico
    pthread_mutex_unlock(&(chargepoint->lock));

    return myPosition;
}



// Reserva ponto de carregamento com prioridade e devolve identificador do ponto de carga
int reserveChargePointPriority(Chargepoint *chargepoint)
{
    // Bloqueio do acesso ao recurso crítico
    pthread_mutex_lock(&(chargepoint->lock));
    
    int myPosition = -1;
    chargepoint->waitingThreads++;
    chargepoint->priorityWait++;

    do {
        // Verificação de uma posição de carregamento livre
        for (int i = 0; i < MAX_CHARGEPOINT; i++) 
        {
            if (chargepoint->position[i] == -1) 
            {
                myPosition = i;
                break;
            }
        }

        // Se uma posição livre for encontrada
        if (myPosition != -1) 
        {
            break;
        } 
        pthread_cond_wait(&(chargepoint->priorityWaitCond), &(chargepoint->lock));


    } while (1);

    chargepoint->waitingThreads--;
    chargepoint->priorityWait--;

    // Atualiza a posição de carregamento
    chargepoint->position[myPosition] = myPosition;
    chargepoint->id[myPosition] = pthread_self();

    printf("\nAcesso a recurso crítico thread nº:%d com ID:%ld com prioridade\n", chargepoint->count++, pthread_self());
    for (int i = 0; i < MAX_CHARGEPOINT; i++) {
        printf("\t %d \t %ld\n", chargepoint->position[i], chargepoint->id[i]);
    }

    // Recursos críticos
    chargepoint->nrSpots++;

    // Desbloqueio do acesso ao recurso crítico
    pthread_mutex_unlock(&(chargepoint->lock));

    return myPosition;
}



// Liberta ponto de carregamento
void freeChargePoint(Chargepoint *chargepoint)
{
    // Bloqueamento do acesso ao recurso crítico
    pthread_mutex_lock(&(chargepoint->lock));

    // Obtenção do ID da thread que acabou
    long int id = pthread_self();

    // Libertação da posição do identificador de carregamento
    for (int i = 0; i < MAX_CHARGEPOINT; i++) {
        if (chargepoint->id[i] == id) {
            chargepoint->position[i] = -1;
            chargepoint->id[i] = 0;
            break;
        }
    }

    // Recursos críticos
    chargepoint->nrSpots--;

    // Ativação de threads em espera
    if (chargepoint->priorityWait > 0) {
        pthread_cond_signal(&(chargepoint->priorityWaitCond));
    } else {
        pthread_cond_signal(&(chargepoint->wait));
    }

    // Desbloqueio do acesso ao recurso crítico
    pthread_mutex_unlock(&(chargepoint->lock));
}




/*
//Reserva prioritária de ponto de carregamento
int reserveChargePointPriority(Chargepoint *chargepoint)
{
	printf("FUNCTION PRIORITY\n");
	//Bloqueio do acesso ao recurso critico
	pthread_mutex_lock(&(chargepoint->lock));
	
	//Posição do identificador de carregamento
	int myPosition;
	int priority = 0;// = chargepoint[0].priority;
	printf("Priority init:%d\n", priority);

	do{
		//Procura de valor mais elevado de prioridade
		for(int i=0; i<chargepoint->count; i++)
		{
			if(chargepoint[i].priority > priority)
				priority = chargepoint[i].priority;
		}
		
		printf("Priority selection:%d\n", priority);
		
		//Verficação de uma posição de carregamento livre
		for(int i=0; i<MAX_CHARGEPOINT;i++)
		{
			if(chargepoint->position[i] == -1 && chargepoint[i].priority == priority)
			{
				myPosition = i;
				break;
			}
			if(chargepoint->position[i] == -1)
			{
				myPosition = i;
				break;
			}
		}
		
		if(myPosition != -1)
			break;
		
		//Espera passiva das threads
		pthread_cond_wait(&(chargepoint->wait),&(chargepoint->lock));
	}while(1);
	
	
	
	
	chargepoint->position[myPosition] = myPosition;
	chargepoint->id[myPosition] = pthread_self();
	
	printf("Acesso a recurso critico thread nº:%d com ID:%ld\n",chargepoint->count++, pthread_self());
	for(int i=0;i<MAX_CHARGEPOINT; i++)
	{
		printf("\t %d \t %ld\n", chargepoint->position[i],chargepoint->id[i]);
	}
	
	//Recursos criticos
	chargepoint->nrSpots++;


	//Desbloqueio do acesso ao recurso critico	
	pthread_mutex_unlock(&(chargepoint->lock));
	
	return myPosition;
}
*/




//Inicialização dos pontos de carregamento
void chargepoint_init (Chargepoint *chargepoint)
{
	//Atualização das variáveis de apoio a zero
	for(int i=0;i<MAX_CHARGEPOINT;i++)
	{
		chargepoint->position[i] = -1;
		chargepoint->id[i] = 0;
	}
	chargepoint->nrSpots = 0;
	chargepoint->count = 0;
	chargepoint->priorityWait = 0;
 
	
	//Inicialização dos mecanismos de sincronismo
	pthread_mutex_init(&(chargepoint->lock), NULL);
	pthread_cond_init(&(chargepoint->wait), NULL);
	
}





//Fim dos pontos de carregamento
void chargepoint_destroy (Chargepoint *chargepoint)
{
	//Finalização dos mecanismos de sincronização
	pthread_mutex_destroy(&(chargepoint->lock));
	pthread_cond_destroy(&(chargepoint->wait));
}



void* thread_func(void * chargepoint)
{
	Chargepoint * point = (Chargepoint*) chargepoint;
	int secs = rand() % 5;
	pthread_t thId = pthread_self();
	int priority = rand() % 2;

	printf("[%ld] - Sleeping for %d secs - Priority(%d)\n", thId, secs, priority);
	secs = 4;
	sleep(secs);
	
	int ticket;

	
	if(priority == 0)
		ticket = reserveChargePoint(point);
	else
		ticket = reserveChargePointPriority(point);
		
		
	printf("[%ld] - Parking for %d secs in postion (%d|%d) - Priority(%d)\n", thId, secs, ticket, MAX_CHARGEPOINT, priority);
	sleep(secs);
	printf("[%ld] - Unparked            in postion (%d|%d) - Priority(%d)\n", thId, ticket, MAX_CHARGEPOINT, priority);
	freeChargePoint(point);
	
	return NULL;
}




int main() 
{
	pthread_t robot[MAX_ROBOT];
	
	Chargepoint point;
	chargepoint_init(&point);
	
	
	//Criação das threads representativas dos robos
	for(int i = 0; i < MAX_ROBOT; ++i)
	{
		pthread_create(robot + i, NULL, thread_func,&point);
	}
	
	//Espera pela terminação das threads
	for(int i = 0; i < MAX_ROBOT; ++i)
	{
		pthread_join(robot[i], NULL);
	}
	
	//sleep(5);
	
	/*
	printf("Alocação de lugares final\n");
	for(int i=0;i<MAX_CHARGEPOINT; i++)
	{
		printf("\t %d", point.position[i]);
		printf("\t %ld\n", point.id[i]);
	}
	* */
	
	chargepoint_destroy(&point);
	
	return 0;
}

