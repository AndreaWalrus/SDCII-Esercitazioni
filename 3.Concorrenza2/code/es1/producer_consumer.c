#include <string.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>       // nanosleep()
#include "common.h"

#define BUFFER_SIZE         128
#define INITIAL_DEPOSIT     0
#define MAX_TRANSACTION     1000
#define NUM_CONSUMERS       10
#define NUM_PRODUCERS       5
#define PRNG_SEED           1000

#define NUM_OPERATIONS      400
#define OPS_PER_CONSUMER    (NUM_OPERATIONS/NUM_CONSUMERS)
#define OPS_PER_PRODUCER    (NUM_OPERATIONS/NUM_PRODUCERS)

// we use the preprocessor to check if our parameters are okay
#if OPS_PER_CONSUMER*NUM_CONSUMERS != OPS_PER_PRODUCER*NUM_PRODUCERS
#error "Choose NUM_CONSUMERS and NUM_PRODUCERS so that we get exactly NUM_OPERATIONS operations"
#endif

// struct used to specify arguments for a thread
typedef struct {
    int threadId;
    int numOps;
} thread_args_t;

// semaphores
sem_t sem1, sem2, n, e;

// shared data
int transactions[BUFFER_SIZE];
int deposit = INITIAL_DEPOSIT;
int read_index, write_index;

// generates a number between -MAX_TRANSACTION and +MAX_TRANSACTION
static inline int performRandomTransaction() {
    struct timespec pause = {0};
    pause.tv_nsec = 10000000; // 10 ms (100*10^6 ns)
    nanosleep(&pause, NULL);

    int amount = rand() % (2 * MAX_TRANSACTION); // {0, ..., 2*MAX_TRANSACTION - 1}
    if (amount >= MAX_TRANSACTION) {
        return MAX_TRANSACTION - (amount+1); // {-MAX_TRANSACTION, ..., -1}
    } else {
        return amount + 1; // {1, ..., MAX_TRANSACTION}
    }
}

// producer thread
void* performTransactions(void* x) {
    thread_args_t* args = (thread_args_t*)x;
    printf("Starting producer thread %d\n", args->threadId);

    while (args->numOps > 0) {
        // produce the item
        int currentTransaction = performRandomTransaction();
        printf("current transaction:%d\n", currentTransaction);

        if(sem_wait(&e)!=0) handle_error("Error in semaphore locking");
        if(sem_wait(&sem1)!=0) handle_error("Error in semaphore locking");

        //printf("producing:%d\n", args->threadId);
        
        // write the item and update write_index accordingly
        transactions[write_index] = currentTransaction;
        write_index = (write_index + 1) % BUFFER_SIZE;

        if(sem_post(&sem1)!=0) handle_error("Error in semaphore unlocking");
        if(sem_post(&n)!=0) handle_error("Error in semaphore unlocking");

        args->numOps--;
        printf("P %d\n", args->numOps);
    }

    free(args);
    pthread_exit(NULL);
}

// consumer thread
void* processTransactions(void* x) {
    thread_args_t* args = (thread_args_t*)x;
    printf("Starting consumer thread %d\n", args->threadId);

    while (args->numOps > 0) {
        
        if(sem_wait(&n)!=0) handle_error("Error in semaphore locking");
        if(sem_wait(&sem2)!=0) handle_error("Error in semaphore locking");
        //printf("consuming:%d\n", args->threadId);
        
        // consume the item and update (shared) variable deposit
        deposit += transactions[read_index];
        read_index = (read_index + 1) % BUFFER_SIZE;
        if (read_index % 100 == 0)
			printf("After the last 100 transactions balance is now %d.\n", deposit);
        
        if(sem_post(&sem2)!=0) handle_error("Error in semaphore unlocking");
        if(sem_post(&e)!=0) handle_error("Error in semaphore unlocking");

        args->numOps--;
        printf("C %d\n", args->numOps);
    }

    free(args);
    pthread_exit(NULL);
}

int main(int argc, char* argv[]) {
    printf("Welcome! This program simulates financial transactions on a deposit.\n");
    printf("\nThe maximum amount of a single transaction is %d (negative or positive).\n", MAX_TRANSACTION);
    printf("\nInitial balance is %d. Press CTRL+C to quit.\n\n", INITIAL_DEPOSIT);

    // initialize read and write indexes
    read_index  = 0;
    write_index = 0;

    // set seed for pseudo-random number generator: we use this to make
    // this code yield the same result across different runs, as long
    // as they are race-free and you make no mistakes :-)
    srand(PRNG_SEED);

    int ret;
    pthread_t producer[NUM_PRODUCERS], consumer[NUM_CONSUMERS];

    // initialize semaphores
    ret = sem_init(&sem1, 0, 1);
    if (ret!=0) handle_error_en(ret, "Error in semaphore initialization");
    
    ret = sem_init(&sem2, 0, 1);
    if (ret!=0) handle_error_en(ret, "Error in semaphore initialization");

    ret = sem_init(&n, 0, 0);
    if (ret!=0) handle_error_en(ret, "Error in semaphore initialization");

    ret = sem_init(&e, 0, BUFFER_SIZE);
    if (ret!=0) handle_error_en(ret, "Error in semaphore initialization");

    int i;
    for (i=0; i<NUM_PRODUCERS; ++i) {
        thread_args_t* arg = malloc(sizeof(thread_args_t));
        arg->threadId = i;
        arg->numOps = OPS_PER_PRODUCER;

        ret = pthread_create(&producer[i], NULL, performTransactions, arg);
        if (ret != 0)  handle_error_en(ret,"Error in pthread create (producer)");
    }

    int j;
    for (j=0; j<NUM_CONSUMERS; ++j) {
        thread_args_t* arg = malloc(sizeof(thread_args_t));
        arg->threadId = j;
        arg->numOps = OPS_PER_CONSUMER;

        ret = pthread_create(&consumer[j], NULL, processTransactions, arg);
        if (ret != 0) handle_error_en(ret,"Error in pthread create (consumer)");
    }

    // join on threads
    for (i=0; i<NUM_PRODUCERS; ++i) {
        ret = pthread_join(producer[i], NULL);
        if (ret != 0) handle_error_en(ret,"Error in pthread join (producer)");
    }

    for (j=0; j<NUM_CONSUMERS; ++j) {
        ret = pthread_join(consumer[j], NULL);
        if (ret != 0) handle_error_en(ret,"Error in pthread join (consumer)");
    }

    printf("Final value for deposit: %d\n", deposit);

    // destroy semaphores
    ret = sem_destroy(&sem1);
    if (ret!=0) handle_error_en(ret, "Error in semaphore destroy");
    ret = sem_destroy(&sem2);
    if (ret!=0) handle_error_en(ret, "Error in semaphore destroy");
    ret = sem_destroy(&n);
    if (ret!=0) handle_error_en(ret, "Error in semaphore destroy");
    ret = sem_destroy(&e);
    if (ret!=0) handle_error_en(ret, "Error in semaphore destroy");

    exit(EXIT_SUCCESS);
}
