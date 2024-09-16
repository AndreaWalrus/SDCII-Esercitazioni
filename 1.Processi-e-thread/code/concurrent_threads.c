#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>

#define N 1000 // number of threads
#define M 10000 // number of iterations per thread
#define V 1 // value added to the balance by each thread at each iteration

//Queue Structure
typedef struct queue_node t_queue_Node;

typedef struct queue{
	t_queue_Node* Head;
	t_queue_Node* Tail;
}t_queue;

struct queue_node{
	pthread_t* thread;
	t_queue_Node* next;
};

//Queue Methods
t_queue* create_queue(){
	t_queue* Queue = (t_queue *)malloc(sizeof(t_queue));
}

void delete_queue(t_queue* Queue){
	t_queue_Node* Head = Queue -> Head;
	t_queue_Node* temp=Head;
	while(temp!=NULL){
		temp=Head->next;
		free(Head);
		Head=Head->next;
	}
	free(Queue);
}

void enqueue(t_queue* Queue, pthread_t* thread){
	t_queue_Node* Tail = Queue->Tail;
	if(Tail==NULL){
		t_queue_Node* temp = (t_queue_Node*)malloc(sizeof(t_queue_Node));
		temp->thread=thread;
		temp->next=NULL;
		Queue->Head=temp;
		Queue->Tail=temp;
	}else{
		t_queue_Node* temp = (t_queue_Node*)malloc(sizeof(t_queue_Node));
		temp->thread=thread;
		temp->next=NULL;
		Tail->next=temp;
		Queue->Tail=Tail;
	}

}

pthread_t* dequeue(t_queue* Queue){
	t_queue_Node* Head = Queue->Head;
	if(Head==NULL) return NULL;
	pthread_t* temp=Head->thread;
	Queue->Head=Head->next;
	free(Head);
	return temp;
}

pthread_t* get_head(t_queue* Queue){
	if(Queue->Head!=NULL) return Queue->Head->thread;
	else return NULL;
}

void print_queue(t_queue* Queue){
	t_queue_Node* Head = Queue->Head;
	while(Head!=NULL){
		printf("id:%ln\n", Head->thread);
		Head=Head->next;
	}
}

unsigned long int shared_variable;
int n = N, m = M, v = V;

void* thread_work(void *arg) {
	int i;
	for (i = 0; i < m; i++)
		shared_variable += v;
	return NULL;
}

int main(int argc, char **argv)
{
	if (argc > 1) n = atoi(argv[1]);
	if (argc > 2) m = atoi(argv[2]);
	if (argc > 3) v = atoi(argv[3]);
	shared_variable = 0;

	//queue allocation


	printf("Going to start %d threads, each adding %d times %d to a shared variable initialized to zero...", n, m, v); fflush(stdout);
	pthread_t* threads = (pthread_t*)malloc(n * sizeof(pthread_t)); // also calloc(n,sizeof(pthread_t))
	int i;
	for (i = 0; i < n; i++)
		if (pthread_create(&threads[i], NULL, thread_work, NULL) != 0) {
			fprintf(stderr, "Can't create a new thread, error %d\n", errno);
			exit(EXIT_FAILURE);
		}
	printf("ok\n");

	printf("Waiting for the termination of all the %d threads...", n); fflush(stdout);
	for (i = 0; i < n; i++)
		pthread_join(threads[i], NULL);
	printf("ok\n");

	unsigned long int expected_value = (unsigned long int)n*m*v;
	printf("The value of the shared variable is %lu. It should have been %lu\n", shared_variable, expected_value);
	if (expected_value > shared_variable) {
		unsigned long int lost_adds = (expected_value - shared_variable) / v;
		printf("Number of lost adds: %lu\n", lost_adds);
	}

    free(threads);

	return EXIT_SUCCESS;
}

