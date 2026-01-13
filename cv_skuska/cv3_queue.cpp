#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <string.h>
#include <errno.h>

#define N 4
#define MAX_STR 256
#define MAX_MSG 8
#define TOTAL 20

static char queue[N][MAX_STR];
static int head = 0;
static int tail = 0;
int line_no = 0;

static sem_t mutex;
static sem_t empty;
static sem_t full;

void *consumer(void *arg) {
    (void)arg;

    for(;;){
        
        sem_wait(&full);
        sem_wait(&mutex);

        //strncpy(msg, queue[head], );
        if(strcmp(queue[head], "quit") == 0) {
            sem_post(&mutex);
            sem_post(&empty);
            head = (head + 1) % N;

            break;
        }
        dprintf(STDOUT_FILENO, "C %d: %s\n",line_no, queue[head]);

        queue[head][0] = '\0';
        head = (head + 1) % N;

        sem_post(&mutex);
        sem_post(&empty);

        line_no++;

        usleep(50*1000);
    }
    return NULL;
}


void *producer(void *arg){
    (void)arg;
    int i = 0; 
    int line_len = 0;
    char line[MAX_STR];
    char buf[MAX_MSG];
    int n = 0;

    while((n = read(STDIN_FILENO, buf, sizeof(buf))) > 0){
        for(int i = 0; i < n; i++) {
            if(buf[i] == '\n') {

                sem_wait(&empty);
                sem_wait(&mutex);

                strncpy(queue[tail], line, line_len);
                queue[tail][line_len] = '\0';
                tail = (tail + 1) % N;
                line[0] = '\0';
                line_len = 0;

                sem_post(&mutex);
                sem_post(&full); 
            }
            else{
                line[line_len++] = buf[i];
            }
        }
        usleep(500*1000);
    }

    sem_wait(&empty);
    sem_wait(&mutex);

    strcpy(queue[tail], "quit");
    tail = (tail + 1) % N;
    line[0] = '\0';
    line_len = 0;

    sem_post(&mutex);
    sem_post(&full); 
    return NULL;
}


int main(){

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