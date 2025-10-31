#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <time.h>
#include <string.h>
#include <sys/wait.h>

bool is_valid_file(const char *f, struct stat i)
{
    if (access(f, X_OK) == 0)
        return false;
    if (access(f, R_OK) != 0)
        return false;
    if (S_ISDIR(i.st_mode))
        return false;
    return true;
}

void print_info(const char *f, struct stat i)
{
    printf("%s : mode: %o, velkost: %lld, cas modifikace: %.24s, ", f, i.st_mode & 0777, (long long)i.st_size, ctime(&i.st_mtime));
    printf("je validny: ");
    if (is_valid_file(f, i))
    {
        printf("ANO\n");
    }
    else
    {
        printf("NIE\n");
    }
}

void new_data(const int *file)
{

    char buf[1024];
    int r;
    while ((r = read(*file, buf, sizeof(buf))) > 0)
    {
        printf("Nove data: %s\n", buf);
    }
}

int main(int argc, char *argv[])
{

    struct stat info;
    char *valid_files[50];
    int count = 0;
    int fd[50][2];
    int logger[2];
    pid_t p;
    pid_t children[50];

    for (int i = 1; i < argc; i++)
    {
        stat(argv[i], &info);
        printf("===============\n");
        if (is_valid_file(argv[i], info))
        {
            valid_files[count++] = argv[i];
        }
    }
    //=======================

    for (int i = 0; i < count; i++)
    {
        if (pipe(fd[i]) == -1)
        {
            perror("pipe chyba");
            return 1;
        }
        p = fork();

        if (p == -1)
        {
            perror("fork zlyhal");
            return 1;
        }

        if (p == 0)
        {
            // child
            close(fd[i][1]);
            close(logger[0]);
            struct stat child_stat;
            stat(valid_files[i], &child_stat);
            for (int k = 0; k < i; k++)
            {
                close(fd[k][0]);
                close(fd[k][1]);
            }

            int f = open(valid_files[i], O_RDONLY);
            lseek(f, 0, SEEK_END);
            char buf[6];

            while (1)
            {
                size_t r = read(fd[i][0], buf, sizeof(buf));
                if (r == 0)
                {
                    break;
                }
                if (strncmp(buf, "check\n", 6) == 0)
                {
                    print_info(valid_files[i], child_stat);
                    new_data(&f);
                    //lopgger
                }
            }
            close(f);
            close(fd[i][0]);
            _exit(0);
        }

        if (p > 0)
        {
            // parent
            close(fd[i][0]);
            children[i] = p;
        }
    }

    while (1)
    {
        if (access("stop", F_OK) == 0)
        {
            printf("Subor stop zastavil program");
            break;
        }
        for (int i = 0; i < count; i++)
        {
            size_t w = write(fd[i][1], "check\n", strlen("check\n"));
        }
        sleep(3);
    }
    for (int i = 0; i < count; i++)
    {
        close(fd[i][1]);
    }
    for (int i = 0; i < count; i++)
    {
        waitpid(children[i], NULL, 0);
    }

    return 0;
}