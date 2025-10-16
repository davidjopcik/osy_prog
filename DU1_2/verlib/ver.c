#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>

bool isValidBankNumber(long S, long weight[], bool onlyValid)
{
    long num = S;
    int sum = 0;
    bool isValid = false;
    int i = 0;

    while (num > 0)
    {
        sum += (num % 10) * weight[i];
        num /= 10;
        i++;
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