#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

void generator(long S, long N, bool binary)
{
    long num = S;
    for (int i = 0; i < N; i++)
    {
        if (binary)
        {
            write(1, &num, sizeof(num));
        }
        else
        {
            printf("%010ld\n", num);
        }

        num += 1;
    }
}

int main(int argc, char *argv[])
{
    char *end;
    long S = strtol(argv[1], &end, 10);
    long N = argc < 3 ? 1000 : atoi(argv[2]);
    bool binary = false;
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-b") == 0)
        {
            binary = true;
        }
    }

    generator(S, N, binary);

    return 0;
}