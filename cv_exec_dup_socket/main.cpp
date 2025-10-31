#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    pid_t pid = fork();

    if (pid == 0) {
        execlp("ls", "ls", "-l", NULL);
        perror("exec"); // ak sa sem dostaneš, exec zlyhal
    } else {
        wait(NULL);
        printf("Rodič: dieťa ukončilo prácu.\n");
    }
}