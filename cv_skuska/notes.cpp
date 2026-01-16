#include <iostream>

int main() {
//vynuluj buffer
memset(animal, '\0', 256);

//porovnanie 2 stringov
if(strcmp(animal, "DOG") == 0)

//porovnaj prvych 7 znakov
if(!strncmp(buf, "COMPILE", 7))

//kopirovanie stringu od pozicie 8
strncpy(animal, buf + 8, sizeof(buf))

//otvorenie suboru a citanie
int f = open("animal.cpp", O_RDONLY);

//otvorenie suboru a zapis
int nf = open("new.cpp", O_CREAT | O_WRONLY | O_TRUNC, 0644);

//POSIX semafory
//Pomenovany otvoreny semafor - ked su forky
#include <semaphore.h>
#include <fcntl.h>

sem_t *sem = sem_open("/mysem", O_CREAT, 0666, 1);

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








    return 0;
}
