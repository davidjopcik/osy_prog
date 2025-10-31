#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

int main() {

    int fd1[2];
    int fd2[2];
    char input_str[100];
    char add_str[] = " David";
    pid_t p;

    printf("Zadaj vstup: ");
    //char input_str[] = "Ahoj";
    scanf("%s", input_str);
    pipe(fd1);
    pipe(fd2);

    p = fork();

    if (p < 0)
    {
        fprintf(stderr, "fork failed");
    }
    else if (p > 0)
    {
        char concat_str[100];
        close(fd1[0]);
        write(fd1[1], input_str, strlen(input_str) + 1);
        close(fd1[1]);
        wait(NULL);

        close(fd2[1]);
        read(fd2[0], concat_str, 100);
        printf("Concated string: %s", concat_str);
        close(fd2[0]);

    }
    else 
    {
        close(fd1[1]);
        char concat_str[100];
        read(fd1[0], concat_str, 100);
        printf("======= %s =====\n", concat_str);
        int k = strlen(concat_str);

        for (int i = 0; i < strlen(add_str); i++)
        {
            concat_str[k++] = add_str[i];
        }
        concat_str[k] = '\0';
        close(fd1[0]);
        close(fd2[0]);

        write(fd2[1], concat_str, strlen(concat_str)+1);
        close(fd2[1]);
        exit(0);
    }
}