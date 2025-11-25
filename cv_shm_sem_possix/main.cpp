#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <fcntl.h>      // O_CREAT, O_RDWR, O_EXCL
#include <sys/stat.h>   // mode_t, 0666
#include <sys/mman.h>   // shm_open, mmap, PROT_*, MAP_*
#include <semaphore.h>  // sem_open, sem_wait, sem_post
//#include <mqueue.h>     // mq_open, mq_send, mq_receive
#include <sys/wait.h>   // wait

typedef struct {
    int head;
    int tail;
    int capacity;
    int data[256];
} shm_buffer_t;

void sem_open_test(){
    const char *SEM_NAME = "/sem_open_test";

    sem_t *sem = sem_open(SEM_NAME, O_CREAT, 0666, 1);

    printf("Cakam na semafor...\n");
    printf("Som v kritickej sekcii...\n");
    sleep(3);
    printf("Opustam kriticku sekciu...\n");
}



int main() {

    sem_open_test();

    return 0;
}

