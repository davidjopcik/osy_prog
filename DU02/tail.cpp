#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <string.h>
#include <time.h>

void file_info(char *input_file)
{
    struct stat mystat;
    stat(input_file, &mystat);
    char *t = asctime(localtime(&mystat.st_mtime));
    printf("mode= %o mtime=%s size= %lld name=%s\n",
           mystat.st_mode, t, mystat.st_size, input_file);
}

int is_valid_file(char *input_file)
{
    struct stat statinfo;
    if (stat(input_file, &statinfo) == -1)
    {
        printf("stat zlyhal\n");
        return 0;
    }
    if (S_ISDIR(statinfo.st_mode))
    {
        fprintf(stderr, "%s: je adresár\n", input_file);
        return 0;
    }
    if (access(input_file, X_OK) == 0)
    {
        printf("%s: je spustitelny\n", input_file);
        return 0;
    }
    if (access(input_file, R_OK) != 0)
    {
        printf("%s: je necitatelny\n", input_file);
        return 0;
    }
    return 1;
}

void child_read(char *input_file, int pipe_r)
{
    FILE *f = fopen(input_file, "r");
    struct stat mystat;
    stat(input_file, &mystat);
    file_info(input_file);

    char buf[16];

    while (1)
    {
        int r_pipe = read(pipe_r, buf, sizeof(buf));
        if (r_pipe == 0)
        {
            break;
        }
        if (r_pipe < 0)
        {
            continue;
        }

        if (r_pipe >= 6 && strncmp(buf, "check\n", 6) == 0)
        {
            stat(input_file, &mystat);
            file_info(input_file);
            rewind(f);
            char chunk[4096];
            int n;
            while ((n = fread(chunk, 1, sizeof(chunk), f)) > 0)
            {
                fwrite(chunk, 1, n, stdout);
            }
            fflush(stdout);
            printf("===============\n");
        }
    }

    fclose(f);
    close(pipe_r);
}

int main(int argc, char **argv)
{

    char *valid_files[50];
    int count = 0;

    for (int i = 1; i < argc; i++)
    {
        if (is_valid_file(argv[i]))
        {
            printf("Ok - %s ", argv[i]);
            file_info(argv[i]);
            printf("-----------------\n");
            valid_files[count++] = argv[i];
        }
        
    }

    int pipes[50][2];
    int pids[50];

    for (int i = 0; i < count; i++)
    {
        pipe(pipes[i]);
        int pid = fork();

        if (pid == 0)
        {
            close(pipes[i][1]);
            for (int k = 0; k < i; k++)
            {
                close(pipes[k][0]);
                close(pipes[k][1]);
            }
            child_read(valid_files[i], pipes[i][0]);
            _exit(0);

        }
        else
        {
            pids[i] = pid;
            close(pipes[i][0]);
        }
    }

    while (1)
    {
        if (access("stop", F_OK) == 0)
        {
            for (int i = 0; i < count; i++)
            {
                close(pipes[i][1]);
            }
            break;
        }
        for (int i = 0; i < count; i++)
        {
            (void)write(pipes[i][1], "check\n", 6);
        }
        sleep(1);
    }

    for (int i = 0; i < count; ++i)
    {
        int status = 0;
        waitpid(pids[i], &status, 0);
    }
    printf("Subor stop zastavil program\n");

    return 0;
}