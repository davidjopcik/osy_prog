#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>

#define QSIZE 5

int buffer[QSIZE];
int head = 0;          // pozícia na čítanie
int tail = 0;          // pozícia na zápis
int count = 0;         // koľko je práve prvkov

pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t not_empty = PTHREAD_COND_INITIALIZER;
pthread_cond_t not_full  = PTHREAD_COND_INITIALIZER;

/* ================= PRODUCER ================= */
void* producer(void* _) {
    for (int i = 1; i <= 15; ++i) {       // vyrobíme 15 hodnôt
        pthread_mutex_lock(&m);

        // čakaj, kým je buffer plný
        while (count == QSIZE)
            pthread_cond_wait(&not_full, &m);

        // vlož novú položku
        buffer[tail] = i;
        tail = (tail + 1) % QSIZE;
        count++;

        printf("[P] vyrobil %d (count=%d)\n", i, count);

        // oznám, že pribudli dáta
        pthread_cond_signal(&not_empty);
        pthread_mutex_unlock(&m);
        usleep(100 * 1000); // simulácia práce
    }
    return NULL;
}

/* ================= CONSUMER ================= */
void* consumer(void* _) {
    for (;;) {
        pthread_mutex_lock(&m);

        // čakaj, kým je buffer prázdny
        while (count == 0)
            pthread_cond_wait(&not_empty, &m);

        int v = buffer[head];
        head = (head + 1) % QSIZE;
        count--;

        printf("    [C] spotreboval %d (count=%d)\n", v, count);

        // oznám, že je voľné miesto
        pthread_cond_signal(&not_full);
        pthread_mutex_unlock(&m);

        usleep(200 * 1000); // pomalší consumer
    }
    return NULL;
}

int main(void) {
    pthread_t p, c;
    pthread_create(&p, NULL, producer, NULL);
    pthread_create(&c, NULL, consumer, NULL);

    pthread_join(p, NULL);

    // consumer necháme chvíľu dobehnúť a potom ukončíme proces
    sleep(2);
    printf("Hotovo.\n");
    return 0;
}