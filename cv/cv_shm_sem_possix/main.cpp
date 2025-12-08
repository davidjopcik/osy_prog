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
    sem_unlink(SEM_NAME);
}

void demo_shm_open_mmap() {
    const char *SHM_NAME = "/demo_shm_open";

    int fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    size_t size = sizeof(shm_buffer_t);
    ftruncate(fd, size);

    shm_buffer_t *buf = (shm_buffer_t *)mmap(NULL, size, PROT_READ |PROT_WRITE, MAP_SHARED, fd, 0);

    buf->head = 0;
    buf->tail = 0;
    buf->capacity = 8;
    buf->data[0] = 123;

     printf("Do shared memory som zapisal hodnotu data[0] = %d\n", buf->data[0]);

    close(fd);
}

void demo_mq_open() {
    const char *MQ_NAME = "/demo_mq_open";

    struct mq_attr attr;
    memset(&attr, 0, sizeof(attr));

    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = 256;
    attr.mq_curmsgs - 0;

    mqd_t mqd = mq_open(MQ_NAME, O_CREAT | O_RDWR, 0666, &attr);
    pid_t pid = fork();

    if(pid == 0) {
        char buf[256];
        unsigned int prio = 0;

        printf("Child: cakam na spravu...\n");
        ssize_t n = mq_receive(mqd, buf, sizeof(buf), &prio);
        buf[n] = '\0';
        printf("Child: sprava prijata: '%s' (n=%zd, prio=%u)\n", buf, n, &prio);
        mq_close(mqd);
        exit(0);
    }
    else {
        const char *msg = "Ahoj z mq_send!\n";
        sleep(1);
        mq_send(mqd, msg, strlen(msg) + 1, 5);
        printf("Parent: Sprava odoslana\n");
        mq_close(mqd);
        mq_unlink(MQ_NAME);
        wait(NULL);
    }
}


int main() {

    //sem_open_test();
    //demo_shm_open_mmap();
    demo_mq_open();

    return 0;
}

