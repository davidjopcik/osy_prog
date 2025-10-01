#include <cstdio>
#include <cstdlib>

void generatorNum(long *S, long *weight, int *N)
{
    long bankNum = *S;
    long newBankNum = *S;
    int count = 0;

    while (count < *N)
    {
        long rest = newBankNum;
        int sum = 0;
        int j = 0;

        while (rest > 0)
        {
            rest = rest % 10;
            sum += rest * weight[j];
            printf("Sum: %d\n", sum);
            newBankNum /= 10;
            j++;
            printf("BankNum: %ld\n", newBankNum);

        }

        if (sum % 11 == 0)
        {
            count++;
            printf("%ld \n", newBankNum);
        }
        newBankNum += 1;
    };
};

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
        printf("N: %d\n", N);
    }

    generatorNum(&S, weight, &N);

    return 0;
}