#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>

int counter = 0;
int value = 0;
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t not_empty  = PTHREAD_COND_INITIALIZER;
pthread_cond_t not_full = PTHREAD_COND_INITIALIZER;

bool has_value = false;


void* producer(void* _) {
    for (int i = 0; i < 5; i++)
    {
        pthread_mutex_lock(&m);
        while (has_value)
        {
            pthread_cond_wait(&not_full, &m);
        }
        value = i;
        has_value = true;
        printf("[P] vyrobil %d\n", value);
        pthread_cond_signal(&not_empty);
        pthread_mutex_unlock(&m);
        usleep(50*1000);
    }
    return NULL;
}

void* consumer(void* _) {
    for (int i = 0;i < 5; i++)
    {
        pthread_mutex_lock(&m);
        while (!has_value)
        {
            pthread_cond_wait(&not_empty, &m);
        }
        int v = value;
        has_value = false;
        printf("[C] spotreboval %d\n", v);
        pthread_cond_signal(&not_full);
        pthread_mutex_unlock(&m);
        
    }
    return NULL;
    
}



int main() {

    pthread_t pr, co;
    pthread_create(&pr, NULL, producer, NULL);
    pthread_create(&co, NULL, consumer, NULL);
    pthread_join(pr, NULL);
    pthread_join(co, NULL);
    pthread_mutex_destroy(&m);
    pthread_cond_destroy(&not_empty);
    pthread_cond_destroy(&not_full);

    return 0;
}