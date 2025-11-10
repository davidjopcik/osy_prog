#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

int prime[10] = {2, 3, 5, 7, 11, 13, 17, 19, 23, 29};

void* prime_count(void* arg) {
    int *index = (int*)arg;
    int sum = 0;
    for (int i = 0; i < 5; i++)
    {
        sum += prime[*index + i];
    }
    printf("Local sum: %d\n", sum);
    *(int*)arg = sum;
    return arg;
}

int main() {
    pthread_t p[2];
    for (int i = 0; i < 2; i++)
    {
        int *r = (int*)malloc(sizeof(int));
        *r = i * 5;
        if(pthread_create(&p[i], NULL, prime_count, r) != 0) return 1;
    }

    int globalSum = 0;
    for (int i = 0; i < 2; i++)
    {
        int *r;
        if(pthread_join(p[i], (void**)&r) != 0) return 1;
        globalSum += *r;
        free(r);
    }
    printf("Globl sum: %d", globalSum);
    

    return 0;
}