#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

#define N 4
#define MAX_STR 128
#define TOTAL 10

static char buf[N][MAX_STR];
static int head = 0;
static int tail = 0;

static sem_t mutex;
static sem_t empty;
static sem_t full;

static void put_item(const char *s){
        strncpy(buf[tail], s, MAX_STR-1);
        buf[tail][MAX_STR-1] = '\0';
        tail = (tail + 1) % N;
}

static void get_item(char *out){
    strncpy(out, buf[head], MAX_STR-1);
    out[MAX_STR-1] = '\0';
    head = (head + 1) % N;
}

static void *producer(void *arg){
    //void(arg);
    for (int i = 0; i < TOTAL; i++)
    {
        char msg[MAX_STR];
        snprintf(msg, sizeof(msg), "item-%d", i);

        sem_wait(&empty);
        sem_wait(&mutex);

        put_item(msg);

        sem_post(&mutex);
        sem_post(&full);

        dprintf(STDERR_FILENO, "P: %s\n", msg);
        usleep(50*1000);
    }
    return NULL;
}

static void *consumer(void *arg) {
    for (int i = 0; i < TOTAL; i++)
    {
        char msg[MAX_STR];

        sem_wait(&full);
        sem_wait(&mutex);

        get_item(msg);

        sem_post(&mutex);
        sem_post(&empty);

        printf("C: %s\n", msg);
        fflush(stdout);
        usleep(120*1000);
    }
    return NULL;
    
}

int main(void) {
    sem_init(&mutex, 0, 1);
    sem_init(&empty, 0, N);
    sem_init(&full, 0, 0);

    pthread_t pt, ct;
    pthread_create(&pt, NULL, producer, NULL);
    pthread_create(&ct, NULL, consumer, NULL);

    pthread_join(pt, NULL);
    pthread_join(ct, NULL);

    sem_destroy(&mutex);
    sem_destroy(&empty);
    sem_destroy(&full);

    return 0;
}