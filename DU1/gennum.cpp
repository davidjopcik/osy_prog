#include <cstdio>
#include <cstdlib>

void generatorNum(long *S, long *weight, int *N)
{
    long bankNumber = *S;
    long newBankNumber = *S;
    int count = 0;

    while (count < *N)
    {
        int sum = 0;
        int i = 0;
        bankNumber = newBankNumber;

        while (bankNumber > 0)
        {
            sum += (bankNumber % 10) * weight[i];
            bankNumber /= 10;
            i++;
        }

        if (sum % 11 == 0)
        {
            printf("%ld\n", newBankNumber);
            count++;
        }
        newBankNumber += 1;
    }
}

int main(int argc, char *argv[])
{
    long weight[10] = {1, 2, 4, 8, 5, 10, 9, 7, 3, 6};

    long S;
    int N;

    if (argc > 3 || argc < 2)
    {
        fprintf(stderr, "Zadaj 1 alebo 2 argumenty!\n");
        return 1;
    }

    sscanf(argv[1], "%ld", &S);

    if (argc == 2)
    {
        N = 1000;
    }
    else
    {
        sscanf(argv[2], "%d", &N);
    }

    generatorNum(&S, weight, &N);

    return 0;
}