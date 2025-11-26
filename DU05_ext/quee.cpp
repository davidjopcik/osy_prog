#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <string.h>

#define N 8
#define MAX_STR 256

static char buffer[N][MAX_STR];

static int head = 0;
static int tail = 0;
static sem_t mutex;
static sem_t empty;
static sem_t full;

void init_queue() {
    sem_init(&mutex, 0, 1);
    sem_init(&empty, 0, N);
    sem_init(&full, 0, 0);
}

void *producer(void *arg) {
    char **items = (char**) arg;

    for (int i = 0; i < 8; i++)
    {
    sem_wait(&empty);
    sem_wait(&mutex);
    strncpy(buffer[tail], items[i], MAX_STR-1);
    buffer[tail][MAX_STR-1] = '\0';
    printf("[PROD] %s\n", items[i]);
    tail = (tail+1) % N;
    sem_post(&mutex);
    sem_post(&full);
    sleep(1);
    }
    return NULL;
}

void *consumer(void *arg) {
    char item[MAX_STR];

    for (int i = 0; i < 8; i++)
    {    
    sem_wait(&full);
    sem_wait(&mutex);
    strncpy(item, buffer[head], MAX_STR-1);
    item[MAX_STR-1] = '\0';
    printf("[CONS] %s\n", item);
    head = (head + 1) % N;
    sem_post(&mutex);
    sem_post(&empty);
    sleep(2);
    }
    return NULL;
}

int main() {
    init_queue();

    char *items[] = {"Ahoj",
        "David",
        "Katka",
        "Teo",
        "Noemi",
        "Semafory",
        "OSY",
        "Producer-Consumer test"};

        pthread_t t1, t2;
        pthread_create(&t1, NULL, producer, items);
        pthread_create(&t2, NULL, consumer, NULL);
  
        pthread_join(t1, NULL);
        pthread_join(t2, NULL);
    

    return 0;
}