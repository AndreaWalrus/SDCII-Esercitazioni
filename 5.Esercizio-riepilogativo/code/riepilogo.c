#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <semaphore.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>

// macros for error handling
#include "common.h"

#define N 100   // child process count
#define M 10    // thread per child process count
#define T 3     // time to sleep for main process

#define FILENAME	"accesses.log"

/*
 * data structure required by threads
 */

typedef struct thread_args_s {
    unsigned int child_id;
    unsigned int thread_id;
} thread_args_t;

/*
 * parameters can be set also via command-line arguments
 */
int n = N, m = M, t = T;

/* TODO: declare as many semaphores as needed to implement
 * the intended semantics, and choose unique identifiers for
 * them (e.g., "/mysem_critical_section") */

 #define SEM_CS "/mysem_cs"
 #define SEM_MAIN "/mysem_main"
 #define SEM_CHILD "/mysem_child"

 sem_t * sem_cs, *sem_main, *sem_child;

/* TODO: declare a shared memory and the data type to be placed 
 * in the shared memory, and choose a unique identifier for
 * the memory (e.g., "/myshm") 
 * Declare any global variable (file descriptor, memory 
 * pointers, etc.) needed for its management
 */
#define SHM_NAME "/myshm"

int fd_shm;
int *data;


/*
 * Ensures that an empty file with given name exists.
 */
void init_file(const char *filename) {
    printf("[Main] Initializing file %s...", FILENAME);
    fflush(stdout);
    int fd = open(FILENAME, O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (fd<0) handle_error("error while initializing file");
    close(fd);
    printf("closed...file correctly initialized!!!\n");
}

/*
 * Create a named semaphore with a given name, mode and initial value.
 * Also, tries to remove any pre-existing semaphore with the same name.
 */
sem_t *create_named_semaphore(const char *name, mode_t mode, unsigned int value) {
    printf("[Main] Creating named semaphore %s...", name);
    fflush(stdout);
    int ret;

    sem_unlink(name);

    sem_t* sem = sem_open(name, O_CREAT | O_EXCL, mode, value);
    if(sem==SEM_FAILED) handle_error("sem_open error");

    printf("done!!!\n");
    return sem;
}

void destroy_named_semaphore(const char* name){
    printf("[Main] Destroying named semaphore %s...", name);

    if(sem_unlink(name)!=0) handle_error("sem_unlink error");

    printf("done!!!\n");
}

void initMemory() {
    shm_unlink(SHM_NAME);

    fd_shm = shm_open(SHM_NAME, O_CREAT | O_EXCL | O_RDWR, 0666);
    if(fd_shm==-1) handle_error("shm_open error");

    if(ftruncate(fd_shm, sizeof(int)*n*m)!=0) handle_error("ftruncate error");

    data = mmap(0, sizeof(int)*n*m, PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm, 0);
    if(data==MAP_FAILED) handle_error("mmap error");

    memset(data, 0, sizeof(int)*n*m);
}

void closeMemory() {

    if(munmap(data, sizeof(int)*n*m)!=0) handle_error("munmap error");

    if(close(fd_shm)!=0) handle_error("close error");

    if(shm_unlink(SHM_NAME)!=0) handle_error("shm_unlink error");

}



void parseOutput() {
    // identify the child that accessed the file most times
    int* access_stats = calloc(n, sizeof(int)); // initialized with zeros
    printf("[Main] Opening file %s in read-only mode...", FILENAME);
	fflush(stdout);
    int fd = open(FILENAME, O_RDONLY);
    if (fd < 0) handle_error("error while opening output file");
    printf("ok, reading it and updating access stats...");
	fflush(stdout);

    size_t read_bytes;
    int index;
    do {
        read_bytes = read(fd, &index, sizeof(int));
        if (read_bytes > 0)
            access_stats[index]++;
    } while(read_bytes > 0);
    printf("ok, closing it...");
	fflush(stdout);

    close(fd);
    printf("closed!!!\n");

    int max_child_id = -1, max_accesses = -1;
    for (int i = 0; i < n; i++) {
        printf("[Main] Child %d accessed file %s %d times\n", i, FILENAME, access_stats[i]);
        if (access_stats[i] > max_accesses) {
            max_accesses = access_stats[i];
            max_child_id = i;
        }
    }
    printf("[Main] ===> The process that accessed the file most often is %d (%d accesses)\n", max_child_id, max_accesses);
    free(access_stats);
}

void* thread_function(void* x) {
    thread_args_t *args = (thread_args_t*)x;

    /* TODO: protect the critical section using semaphore(s) as needed */

    // open file, write child identity and close file

    printf("[Child#%d-Thread#%d] Waiting\n", args->child_id, args->thread_id);	
    
    if(sem_wait(sem_cs)!=0) handle_error("sem_wait cs error");

    int fd = open(FILENAME, O_WRONLY | O_APPEND);
    if (fd < 0) handle_error("error while opening file");
    printf("[Child#%d-Thread#%d] File %s opened in append mode!!!\n", args->child_id, args->thread_id, FILENAME);	

    write(fd, &(args->child_id), sizeof(int));
    printf("[Child#%d-Thread#%d] %d appended to file %s opened in append mode!!!\n", args->child_id, args->thread_id, args->child_id, FILENAME);	
    
    close(fd);
    printf("[Child#%d-Thread#%d] File %s closed!!!\n", args->child_id, args->thread_id, FILENAME);

    if(sem_post(sem_cs)!=0) handle_error("sem_post cs error");
    
    free(x);
    pthread_exit(NULL);
}

void mainProcess() {
    /* TODO: the main process waits for all the children to start,
     * it notifies them to start their activities, and sleeps
     * for some time t. Once it wakes up, it notifies the children
     * to end their activities, and waits for their termination.
     * Finally, it calls the parseOutput() method and releases
     * any shared resources. */


    printf("[Main] Waiting for all children to be created\n");
    while(data[1]!=n){
        printf("[Main] %d children created\n", data[1]);
        usleep(100*1000);
    }
    data[2]=1;
    for(int i=0; i<n; i++){
        if(sem_post(sem_main)!=0) handle_error("sem_post main error");
    }

    printf("[Main] All children notified, sleeping...\n");
    sleep(t);
    printf("[Main] Notifying all children to end\n");
    data[0]=1;

    for (int i=0; i<n; ++i) {
        int status;
        int ret = wait(&status);
        if (ret == -1) handle_error("wait");
        if (WEXITSTATUS(status)) handle_error_en(WEXITSTATUS(status), "child crashed");
    }
    printf("[Main] All children ended, parsing output...\n");

    parseOutput();

    closeMemory();
    destroy_named_semaphore(SEM_CS);
    destroy_named_semaphore(SEM_CHILD);
    destroy_named_semaphore(SEM_MAIN);
}

void childProcess(int child_id) {
    /* TODO: each child process notifies the main process that it
     * is ready, then waits to be notified from the main in order
     * to start. As long as the main process does not notify a
     * termination event [hint: use sem_getvalue() here], the child
     * process repeatedly creates m threads that execute function
     * thread_function() and waits for their completion. When a
     * notification has arrived, the child process notifies the main
     * process that it is about to terminate, and releases any
     * shared resources before exiting. */
    
    printf("[Child#%d] Created\n", child_id);
    if(sem_wait(sem_child)!=0) handle_error("sem_wait child error");
    data[1]++;
    if(sem_post(sem_child)!=0) handle_error("sem_post child error");
    printf("[Child#%d] Notified Main\n", child_id);

    while(data[2]!=1);

    printf("[Child#%d] Started\n", child_id);

    pthread_t* threads = (pthread_t*)malloc(m * sizeof(pthread_t));
	int i;
    while(data[0]!=1){
        printf("[Child#%d] Spawning %d threads\n", child_id, m);
        for (i = 0; i < m; i++)
            thread_args_t* args = (thread_args_t*)malloc(sizeof(thread_args_t));
            if (pthread_create(&threads[i], NULL, thread_function, NULL) != 0) {
                fprintf(stderr, "Can't create a new thread, error %d\n", errno);
                exit(EXIT_FAILURE);
            }
            printf("%d",i);
        printf("[Child#%d] Spawned %d threads\n", child_id, m);

        printf("[Child#%d] Waiting for the termination of all the %d threads...\n", child_id , m); fflush(stdout);
        for (i = 0; i < m; i++)
            pthread_join(threads[i], NULL);
        printf("[Child#%d] All threads terminated\n", child_id);
    }    

    printf("[Child#%d] Terminating from main signal!!!\n", child_id);
    free(threads);
}

int main(int argc, char **argv) {
    // arguments
    if (argc > 1) n = atoi(argv[1]);
    if (argc > 2) m = atoi(argv[2]);
    if (argc > 3) t = atoi(argv[3]);

    // initialize the file
    init_file(FILENAME);

    /* TODO: initialize any semaphore needed in the implementation, and
     * create N children where the i-th child calls childProcess(i); 
     * initialize the shared memory (create it, set its size and map it in the 
     * memory), then the main process executes function mainProcess() once 
     * all the children have been created */

    sem_cs = create_named_semaphore(SEM_CS, 0660, 1);
    sem_child = create_named_semaphore(SEM_CHILD, 0660, 1);
    sem_main = create_named_semaphore(SEM_MAIN, 0660, 0);
    
    initMemory();

    int i, ret;
    for (i=0; i<n; ++i) {
        pid_t pid = fork();
        if (pid == -1) {
            handle_error("fork");
        } else if (pid == 0) {
            childProcess(i);
            _exit(EXIT_SUCCESS);
        }
    }
    
    mainProcess();
    exit(EXIT_SUCCESS);
}