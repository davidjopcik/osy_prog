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


void* hello_thread(void* arg) {
    int* a = (int*)arg;
    printf("Ahoj z vlakna, mam cislo: %d\n", *a);
    return NULL;
}

struct BasicArg {
    int a, b;
};

void* sum_thread(void* arg) {
    struct BasicArg *s = static_cast<BasicArg*>(arg);
    return (void*)(intptr_t)(s->a + s->b);
}

void* add(void* _) {
    for (int i = 0; i < 1000000; i++)
    {
        pthread_mutex_lock(&m);
        counter++;
        pthread_mutex_unlock(&m);
    };
    return NULL; 
}


void* producer(void* _) {
    for (int i = 0; i < 5; i++)
    {
        pthread_mutex_lock(&m);
        while (has_value)
        {
            pthread_cond_wait(&not_empty, &m);
        }
        value = i;
        has_value = false;
         pthread_cond_signal(&not_full);
        
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
            pthread_cond_wait(&not_full, &m);
        }
        int v = value;
        has_value = false;
        printf("[C] spotreboval %d\n", v);
        pthread_cond_signal(&not_empty);
        pthread_mutex_unlock(&m);
        
    }
    return NULL;
    
}



int main() {

    pthread_t t1, t2, t3, t4, pr, co;
    int x = 4;
    pthread_create(&t1, NULL, hello_thread, &x);
    void* ret = nullptr;
    pthread_join(t1, &ret);

    BasicArg a1{1, 5};
    pthread_create(&t2, NULL, sum_thread, &a1);
    void* c = nullptr;
    pthread_join(t2, &c);

    printf("Sucet= %d\n", (int)(intptr_t)c);

    printf("============\n");
    pthread_create(&t3, NULL, add, NULL);
    pthread_create(&t4, NULL, add, NULL);
    pthread_join(t3, NULL);
    pthread_join(t4, NULL);
    printf("counter = %d\n", counter);
    pthread_mutex_destroy(&m);
    printf("============\n");


    pthread_create(&pr, NULL, producer, NULL);
    pthread_create(&co, NULL, consumer, NULL);
    pthread_join(pr, NULL);
    pthread_join(co, NULL);
    pthread_mutex_destroy(&m);
    pthread_cond_destroy(&not_empty);
    pthread_cond_destroy(&not_full);



    return 0;
}