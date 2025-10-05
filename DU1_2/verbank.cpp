#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

bool isValidBankNumber(long S, bool onlyValid)
{
    long num = S;
    int sum = 0;
    bool isValid = false;

    while (num > 0)
    {
        sum += num % 10;
        num /= 10;
    }
    isValid = (sum % 11 == 0);

    if (onlyValid == true)
    {
        if (isValid)
        {
            printf("Cislo uctu: %ld - platne\n", S);
        }
    }
    else
    {
        printf("Cislo uctu: %ld - %s ", S, isValid ? "platne\n" : "neplatne\n");
    }

    return isValid;
}

int main(int argc, char *argv[])
{
    if (argc > 2)
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

    char line[11];
    char *end;
    bool onlyValid = false;
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-v") == 0)
        {
            onlyValid = true;
        }
    }

    while (binary == true ? read(0, &line, sizeof(line)) : scanf("%10s", line) == 1)
    {
        long S = strtol(line, &end, 10);
        isValidBankNumber(S, onlyValid);
    }

    return 0;
}