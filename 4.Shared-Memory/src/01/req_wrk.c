#include "common.h"

#include <stdio.h>
#include <stdlib.h>

#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <pthread.h>


// data array
int * data;
/** COMPLETE THE FOLLOWING CODE BLOCK
*
* Add any needed resource 
**/

sem_t* sem_wrk, *sem_req;

int shm_fd;

int request() {
  /** COMPLETE THE FOLLOWING CODE BLOCK
  *
  * map the shared memory in the data array
  **/

  data = (int*)mmap(0,SIZE, PROT_WRITE | PROT_READ, MAP_SHARED, shm_fd, 0);
  if(data==MAP_FAILED) handle_error("mmap error");

  printf("request: mapped address: %p\n", data);

  int i;
  for (i = 0; i < NUM; ++i) {
    data[i] = i;
  }
  printf("request: data generated\n");

   /** COMPLETE THE FOLLOWING CODE BLOCK
    *
    * Signal the worker that it can start the elaboration
    * and wait it has terminated
    **/

  if(sem_post(sem_wrk)!=0) handle_error("sem_post wrk error");
  if(sem_wait(sem_req)!=0) handle_error("sem_wait req error");
  printf("request: acquire updated data\n");

  printf("request: updated data:\n");
  for (i = 0; i < NUM; ++i) {
    printf("%d\n", data[i]);
  }

   /** COMPLETE THE FOLLOWING CODE BLOCK
    *
    * Release resources
    **/
  
  if(munmap(data, SIZE)!=0) handle_error("munmap error");

  return EXIT_SUCCESS;
}

int work() {

  /** COMPLETE THE FOLLOWING CODE BLOCK
  *
  * map the shared memory in the data array
  **/

  data=(int*)mmap(0, SIZE, PROT_WRITE | PROT_READ, MAP_SHARED, shm_fd, 0);
  if(data==MAP_FAILED) handle_error("mmap error");

  printf("worker: mapped address: %p\n", data);

   /** COMPLETE THE FOLLOWING CODE BLOCK
    *
    * Wait that the request() process generated data
    **/

  printf("worker: waiting initial data\n");
  
  if(sem_wait(sem_wrk)!=0) handle_error("sem_wait wrk error");

  printf("worker: initial data acquired\n");

  printf("worker: update data\n");
  int i;
  for (i = 0; i < NUM; ++i) {
    data[i] = data[i] * data[i];
  }

  printf("worker: release updated data\n");

   /** COMPLETE THE FOLLOWING CODE BLOCK
    *
    * Signal the requester that elaboration terminated
    **/

  if(sem_post(sem_req)!=0) handle_error("sem_post req error");


  /** COMPLETE THE FOLLOWING CODE BLOCK
   *
   * Release resources
   **/
  
  if(munmap(data, SIZE)!=0) handle_error("munmap error");

  return EXIT_SUCCESS;
}



int main(int argc, char **argv){

  /** COMPLETE THE FOLLOWING CODE BLOCK
   *
   * Create and open the needed resources 
   **/

  // Semaphores

  sem_unlink(SEM_NAME_WRK);
  sem_unlink(SEM_NAME_REQ);

  sem_wrk=sem_open(SEM_NAME_WRK, O_CREAT | O_EXCL, 0666, 0);
  if(sem_wrk==SEM_FAILED) handle_error("sem_open wrk error");
  
  sem_req=sem_open(SEM_NAME_REQ, O_CREAT | O_EXCL, 0666, 0);
  if(sem_req==SEM_FAILED) handle_error("sem_open req error");

  // Shared Memory
  shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
  if(shm_fd==-1) handle_error("shm_open error");

  if(ftruncate(shm_fd, SIZE)!=0) handle_error("ftruncate error");

  int ret;
  pid_t pid = fork();
  if (pid == -1) {
    handle_error("main: fork");
  } else if (pid == 0) {
    work();
    _exit(EXIT_SUCCESS);
  }

  request();
  int status;
  ret = wait(&status);
  if (ret == -1)
    handle_error("main: wait");
  if (WEXITSTATUS(status)) handle_error_en(WEXITSTATUS(status), "request() crashed");


  /** COMPLETE THE FOLLOWING CODE BLOCK
   *
   * Close and release resources
   **/

  // Semaphores
  if(sem_close(sem_wrk)!=0) handle_error("sem_close wrk error");

  if(sem_close(sem_req)!=0) handle_error("sem_close req error");

  if(sem_unlink(SEM_NAME_WRK)!=0) handle_error("sem_unlink wrk error");

  if(sem_unlink(SEM_NAME_REQ)!=0) handle_error("sem_unlink req error");

  // Shared Memory
  if(close(shm_fd)!=0) handle_error("shm close error");

  if(shm_unlink(SHM_NAME)!=0) handle_error("shm_unlink error");

  _exit(EXIT_SUCCESS);

}
