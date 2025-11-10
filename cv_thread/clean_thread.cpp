#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <time.h>

void* roll_dice(void* arg) {
    int *result = (int*)malloc(sizeof(int));
    *result = (rand() % 6) + 1;
    // printf("%d\n", value);
    printf("Thread result: %d\n", *result);
    return result;
}

int main(int argc, char* argv[]) {
    int* res;
    srand(time(NULL));
    pthread_t th;
    if (pthread_create(&th, NULL, roll_dice, NULL) != 0) {
        return 1;
    }
    if (pthread_join(th, (void**) &res) != 0) {
        return 2;
    }
    printf("Main res: %d\n", (void*)res);
    printf("Result: %d\n", *res);
    free(res);
    return 0;
}