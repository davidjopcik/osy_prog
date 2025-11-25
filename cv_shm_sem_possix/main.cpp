#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <fcntl.h>      // O_CREAT, O_RDWR, O_EXCL
#include <sys/stat.h>   // mode_t, 0666
#include <sys/mman.h>   // shm_open, mmap, PROT_*, MAP_*
#include <semaphore.h>  // sem_open, sem_wait, sem_post
#include <mqueue.h>     // mq_open, mq_send, mq_receive
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

void demo_shm_open_mmap() {
    const char *SHM_NAME = "/demo_shm_open";

    int fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    size_t size = sizeof(shm_buffer_t);
    ftruncate(fd, size);

    shm_buffer_t *buf = (shm_buffer_t *)mmap(NULL, size, PROT_READ |PROT_WRITE, MAP_SHARED, fd, 0);
    
}



int main() {

    sem_open_test();

    return 0;
}

