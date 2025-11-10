// pc_sem.c
// Jednoduchý príklad PRODUCER–CONSUMER s POSIX semaformi (mutex, empty, full)
// Žiadne kontroly chýb, iba demonštrácia správania.
// Po zadaní riadku sa hneď vypíše, po "exit" sa program ukončí.

#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

#define N 8               // kapacita fronty
#define MAX_STR 256       // max dĺžka jedného reťazca
#define CONSUME_MS 400    // oneskorenie spotreby v ms
#define MS(x) ((x)*1000)  // prevod na mikrosekundy pre usleep

// --- zdieľaný kruhový buffer ---
static char buffer[N][MAX_STR];
static int head = 0;      // index na čítanie
static int tail = 0;      // index na zápis

// --- tri semafory podľa Tanenbauma ---
static sem_t mutex;   // vzájomné vylúčenie
static sem_t empty;   // počet voľných slotov
static sem_t full;    // počet plných slotov

// --- vloženie jedného "item" do fronty ---
void producer(char *item)
{
    sem_wait(&empty);     // čakaj, kým je voľné miesto
    sem_wait(&mutex);     // vstup do kritickej sekcie

    strncpy(buffer[tail], item, MAX_STR - 1);
    buffer[tail][MAX_STR - 1] = '\0';
    tail = (tail + 1) % N;

    sem_post(&mutex);     // odchod z kritickej sekcie
    sem_post(&full);      // pribudol plný slot
}

// --- vybratie jedného "item" z fronty ---
void consumer(char *item)
{
    sem_wait(&full);      // čakaj, kým je niečo vo fronte
    sem_wait(&mutex);     // vstup do kritickej sekcie

    strncpy(item, buffer[head], MAX_STR - 1);
    item[MAX_STR - 1] = '\0';
    head = (head + 1) % N;

    sem_post(&mutex);     // odchod z kritickej sekcie
    sem_post(&empty);     // uvoľnil sa slot
}

// --- vlákno producenta ---
static void *producer_thread(void *arg)
{
    char line[MAX_STR];

    while (1) {
        fputs("> ", stdout); fflush(stdout);   // prompt pre používateľa

        if (!fgets(line, sizeof(line), stdin))
            break;

        size_t L = strlen(line);
        if (L && line[L - 1] == '\n')
            line[L - 1] = '\0';

        if (strcmp(line, "exit") == 0)
            break;

        producer(line);    // vloží jeden riadok
    }

    // pošleme špeciálny záznam pre ukončenie konzumenta
    producer("<<END>>");
    return NULL;
}

// --- vlákno konzumenta ---
static void *consumer_thread(void *arg)
{
    char out[MAX_STR];
    for (;;) {
        consumer(out);

        if (strcmp(out, "<<END>>") == 0)
            break;

        printf("-> %s\n", out);
        fflush(stdout);

        usleep(MS(CONSUME_MS));   // spomalenie výpisu
    }
    return NULL;
}

// --- main ---
int main(void)
{
    sem_init(&mutex, 0, 1);
    sem_init(&empty, 0, N);
    sem_init(&full,  0, 0);

    pthread_t tp, tc;
    pthread_create(&tp, NULL, producer_thread, NULL);
    pthread_create(&tc, NULL, consumer_thread, NULL);

    pthread_join(tp, NULL);
    pthread_join(tc, NULL);

    sem_destroy(&mutex);
    sem_destroy(&empty);
    sem_destroy(&full);
    return 0;
}