#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

int prime[10] = {2, 3, 5, 7, 11, 13, 17, 19, 23, 29};

void * rand_prime(void* arg) {
    int *a = (int*)arg;
    int sum = 0;
    for (int i = 0; i < 5; i++)
    {
        sum += prime[*a + i];
    }
    printf("Sum thread %d is: %d\n", *a, sum);
    *(int*)arg = sum;
    return arg;
}

int main() {

    pthread_t p[2];
    for (int i = 0; i < 2; i++)
    {
        int* a = (int*)malloc(sizeof(int));
        *a = 5 * i; 
        pthread_create(&p[i], NULL, rand_prime, a);
    }
    int gobalSum = 0;

    for (int i = 0; i < 2; i++)
    {
        int* r;
        pthread_join(p[i], (void**)&r);
        gobalSum += *r;
    free(r);

    }
    printf("Global sum is: %d\n", gobalSum);


    return 0;
}