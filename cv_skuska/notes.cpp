#include <iostream>

int main() {

    ```c
memset(buf, 0, size);              // vymazanie pamäte
memcpy(dst, src, n);               // kopírovanie bajtov (NEKONČÍ \0)
strlen(str);                       // dĺžka stringu bez \0
strcmp(a, b);                      // porovnanie stringov
strncmp(a, b, n);                  // porovnanie n znakov
snprintf(buf, size, "%s", str);  // bezpečné skladanie stringu
```
//vynuluj buffer
memset(animal, '\0', 256);

//porovnanie 2 stringov
if(strcmp(animal, "DOG") == 0)

//porovnaj prvych 7 znakov
if(!strncmp(buf, "COMPILE", 7))

//kopirovanie stringu od pozicie 8
strncpy(animal, buf + 8, sizeof(buf))

//pridanie stringu k aktualnemu
strcat(animal, buf)


```c
int day, month;
sscanf(line, "DAY %d.%d", &day, &month);
```

Alebo manuálne:

```c
memcpy(buf, line+4, 2);
buf[2] = '\0';
```

//otvorenie suboru a citanie
int f = open("animal.cpp", O_RDONLY);

//otvorenie suboru a zapis
int nf = open("new.cpp", O_CREAT | O_WRONLY | O_TRUNC, 0644);

//POSIX semafory
//Pomenovany otvoreny semafor - ked su forky
#include <semaphore.h>
#include <fcntl.h>

sem_t *sem = sem_open("/mysem", O_CREAT, 0660, 1);

sem_wait(sem);     // zamkni

// 🔒 kritická sekcia (napr. zápis do súboru)

sem_post(sem);     // odomkni

sem_close(sem);     // zatvorí handle v procese
sem_unlink("/mysem"); // odstráni zo systému (iba raz!)

//Nepomenovany semafor / ked su thready
sem_t mutex;
sem_init(&mutex, 0, 1);   // pshared = 0 → len v rámci procesu (thready)
void* worker(void* arg) {
    sem_wait(&mutex);     // zamkni

    // 🔒 kritická sekcia
    printf("Thread pracuje\n");
    sleep(1);

    sem_post(&mutex);     // odomkni
    return NULL;
}
sem_destroy(&mutex);


//Shared Memory
#define SHM_NAME        "/shm_my"

struct SharedMemory {
    int count;
    char field[5][5];
    sem_t players[2];
};

struct SharedMemory *shmm = NULL;

int my_sem = shm_open( SHM_NAME, O_CREAT | O_RDWR, 0660 );
    ftruncate(my_sem, sizeof(SharedMemory));

    shmm = ( SharedMemory * ) mmap( nullptr, sizeof( SharedMemory ), PROT_READ | PROT_WRITE,
            MAP_SHARED, my_sem, 0 );

sem_init(&shmm->players[0], 1, 1);
sem_init(&shmm->players[1], 1, 0);

shm_unlink( SHM_NAME );


//WAIT status
#include <signal.h>
#include <sys/wait.h>

signal(SIGCHLD, SIG_DFL);   // <-- KRITICKE: aby waitpid v compile_animal fungoval
int status;
waitpid(p, &status, 0);
if(WEXITSTATUS(status)){error}


//exec
execlp("g++", "g++", "-DANIMAL", "animal.cpp", "-o", "animal", (char*)NULL);


//nastavenie prav na spustenie
#include <sys/stat.h>
chmod("PID.bin", 0755); //v kode

chmod +x program.bin
ls -l program.bin
-rwxr-xr-x program.bin

//zisti velkost suboru
int size = lseek(fd, 0, SEEK_END);
lseek(fd, 0, SEEK_SET); // - vrati sa spat na zaiatok




```c
pthread_t t;
pthread_create(&t, nullptr, client_handle, (void*)(intptr_t)l_sock_client);
pthread_join(t, nullptr);

void *client_handle(void *par) {
    int scl = (int)(intptr_t) par;


    close(scl);

    return NULL;
} 

```

```c
pid_t pid = fork();
if (pid == 0) {
    execvp("display", argv);
    exit(1);
}
wait(NULL);
```






    return 0;
}
