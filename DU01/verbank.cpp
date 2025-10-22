#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>



int main(int argc, char *argv[])
{
    if (argc > 3)
    {
        fprintf(stderr, "Neplatny pocet argumentov\n");
        return 1;
    }

    bool binary = false;
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-b") == 0)
        {
            binary = true;
        }
    }
    long weight[11] = {1, 2, 4, 8, 5, 10, 9, 7, 3, 6};
    char line[11];
    char *end;
    bool onlyValid = false;
    long S;
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-v") == 0)
        {
            onlyValid = true;
        }
    }

    while (binary ? (read(0, &S, sizeof(S)) == (ssize_t)sizeof(S))
                  : (scanf("%10s", line) == 1))
    {
        if (!binary)
        {
            S = strtol(line, &end, 10);
            isValidBankNumber(S, weight, onlyValid);
        }
        else
        {
            isValidBankNumber(S, weight, onlyValid);
        }
    }

    return 0;
}