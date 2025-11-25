//***************************************************************************
// OSY – IPC CHEATSHEET (C++ súbor, ale C-štýl kódu)
//
// Obsah:
//  - sem_open   : pojmenovaný semafor pre viac procesov
//  - shm_open   : pojmenovaná zdieľaná pamäť
//  - ftruncate  : nastavenie veľkosti shm/súboru
//  - mmap       : namapovanie shm do adresového priestoru
//  - mq_open    : POSIX message queue
//
// Kód je písaný tak, aby bol čitateľný pre začiatočníka.
// Nepoužívajú sa C++ streamy, len C funkcie (printf, perror, ...).
//***************************************************************************

#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <fcntl.h>      // O_CREAT, O_RDWR, O_EXCL
#include <sys/stat.h>   // mode_t, 0666
#include <sys/mman.h>   // shm_open, mmap, PROT_*, MAP_*
#include <semaphore.h>  // sem_open, sem_wait, sem_post
#include <mqueue.h>     // mq_open, mq_send, mq_receive
#include <sys/wait.h>   // wait

//***************************************************************************
// Jednoduché typy pre shared memory príklad
//***************************************************************************

typedef struct {
    int head;
    int tail;
    int capacity;
    int data[8]; // malý buffer, len pre ukážku
} shm_buffer_t;

//***************************************************************************
// 1) sem_open – pojmenovaný semafor
//
// Ukazuje, ako vytvoriť/otvoriť semafor a použiť sem_wait / sem_post.
//***************************************************************************

void demo_sem_open()
{
    printf("== DEMO: sem_open ==\n");

    const char *SEM_NAME = "/demo_sem_open";

    // Vytvorenie alebo otvorenie semaforu s hodnotou 1 (mutex).
    sem_t *sem = sem_open(SEM_NAME, O_CREAT, 0666, 1);
    if (sem == SEM_FAILED) {
        perror("sem_open");
        exit(1);
    }

    printf("Cakam na semafor...\n");
    if (sem_wait(sem) == -1) {
        perror("sem_wait");
        exit(1);
    }

    printf("Som v kritickej sekcii (drzim semafor)!\n");
    sleep(1); // simulacia prace

    printf("Opustam kriticku sekciu.\n");
    if (sem_post(sem) == -1) {
        perror("sem_post");
        exit(1);
    }

    if (sem_close(sem) == -1) {
        perror("sem_close");
    }

    // POZOR:
    // sem_unlink(SEM_NAME) by zmazal semafor z OS.
    // Typicky sa volá raz v "cleanup" programe, nie v každom procese.
    // sem_unlink(SEM_NAME);
}

//***************************************************************************
// 2) shm_open + ftruncate + mmap – jednoduchý shared memory príklad
//
// Kroky:
//  - shm_open: vytvorí/otvorí shm objekt (ako súbor)
//  - ftruncate: nastaví veľkosť shm objektu
//  - mmap: namapuje shm do pamäte ako pointer na shm_buffer_t
//***************************************************************************

void demo_shm_open_mmap()
{
    printf("\n== DEMO: shm_open + ftruncate + mmap ==\n");

    const char *SHM_NAME = "/demo_shm_open";

    // 1) otvor / vytvor shared memory objekt
    int fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (fd == -1) {
        perror("shm_open");
        exit(1);
    }

    // 2) nastav veľkosť na veľkosť našej štruktúry
    size_t size = sizeof(shm_buffer_t);
    if (ftruncate(fd, size) == -1) {
        perror("ftruncate");
        exit(1);
    }

    // 3) namapuj do pamäte
    shm_buffer_t *buf = (shm_buffer_t *)mmap(NULL, size,
                                             PROT_READ | PROT_WRITE,
                                             MAP_SHARED,
                                             fd, 0);
    if (buf == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }

    // inicializácia bufferu
    buf->head = 0;
    buf->tail = 0;
    buf->capacity = 8;
    buf->data[0] = 123;

    printf("Do shared memory som zapisal hodnotu data[0] = %d\n", buf->data[0]);

    // cleanup mapovania (objekt ako taký v OS zostáva)
    if (munmap(buf, size) == -1) {
        perror("munmap");
    }

    close(fd);

    // Poznámka:
    // shm_unlink(SHM_NAME); by zmazal objekt zo systému.
    // Typicky sa volá raz, keď chceme shm úplne odstrániť.
    // shm_unlink(SHM_NAME);
}

//***************************************************************************
// 3) mq_open – POSIX fronta správ
//
// Jednoduchý príklad s fork():
//  - rodič pošle správu do fronty (mq_send)
//  - dieťa ju prijme (mq_receive)
//***************************************************************************

void demo_mq_open()
{
    printf("\n== DEMO: mq_open + mq_send + mq_receive ==\n");

    const char *MQ_NAME = "/demo_mq_open";

    struct mq_attr attr;
    memset(&attr, 0, sizeof(attr));

    attr.mq_flags   = 0;      // 0 = blokuje pri send/receive
    attr.mq_maxmsg  = 10;     // max 10 správ vo fronte
    attr.mq_msgsize = 256;    // max 256 bajtov na jednu správu
    attr.mq_curmsgs = 0;      // ignorovať pri mq_open

    // otvor / vytvor message queue
    mqd_t mqd = mq_open(MQ_NAME, O_CREAT | O_RDWR, 0666, &attr);
    if (mqd == (mqd_t)-1) {
        perror("mq_open");
        exit(1);
    }

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(1);
    }

    if (pid == 0) {
        // DIEŤA – RECEIVER
        char buf[256];
        unsigned int prio = 0;

        printf("Child: cakam na spravu...\n");
        ssize_t n = mq_receive(mqd, buf, sizeof(buf), &prio);
        if (n == -1) {
            perror("mq_receive");
            exit(1);
        }

        buf[n] = '\0'; // ukončíme string
        printf("Child: prijata sprava: '%s' (n=%zd, prio=%u)\n", buf, n, prio);

        mq_close(mqd);
        exit(0);
    }
    else {
        // RODIČ – SENDER
        const char *msg = "Ahoj z mq_send!";
        sleep(1); // nech má dieťa čas zavolať mq_receive

        if (mq_send(mqd, msg, strlen(msg) + 1, 5) == -1) {
            perror("mq_send");
            exit(1);
        }

        printf("Parent: sprava odoslana.\n");

        mq_close(mqd);
        mq_unlink(MQ_NAME); // zmazanie fronty zo systému

        wait(NULL);
    }
}

//***************************************************************************
// main – jednoduchý prepínač, aby si vedel spustiť jednotlivé demá
//
// Použitie:
//   ./ipc_cheatsheet sem
//   ./ipc_cheatsheet shm
//   ./ipc_cheatsheet mq
//***************************************************************************

int main(int argc, char **argv)
{
    if (argc < 2) {
        printf("Pouzitie:\n");
        printf("  %s sem   - demo sem_open\n", argv[0]);
        printf("  %s shm   - demo shm_open + ftruncate + mmap\n", argv[0]);
        printf("  %s mq    - demo mq_open + mq_send + mq_receive\n", argv[0]);
        return 0;
    }

    if (strcmp(argv[1], "sem") == 0) {
        demo_sem_open();
    }
    else if (strcmp(argv[1], "shm") == 0) {
        demo_shm_open_mmap();
    }
    else if (strcmp(argv[1], "mq") == 0) {
        demo_mq_open();
    }
    else {
        printf("Neznamy prikaz: %s\n", argv[1]);
    }

    return 0;
}