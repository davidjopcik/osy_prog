#include <cstdio>
#include <cstdlib>
#include <string.h>

void verification(long *S, long *weight, bool *valid) {
    long bankNumber = *S;
    long sum = 0;
    int i = 0;

    while (bankNumber > 0)
    {
        sum += (bankNumber % 10) * weight[i];
        bankNumber /= 10;
        i++;
    }
    if ((*valid) && (sum % 11 == 0))
    {
        printf("ucet: %ld: ", *S);
        printf("Platny\n");
    }
    if(!(*valid))
    {
        printf("ucet: %ld: ", *S);
        printf(sum % 11 == 0 ? "Platny\n" : "Neplatny\n");
    }
}

int main(int argc, char *argv[]) {
    long weight[10] = {1, 2, 4, 8, 5, 10, 9, 7, 3, 6};
    long S;
    char line[11];
    char *end;
    bool onlyValid = false;

    if ((argc == 2) && (strcmp(argv[1], "-v") == 0))
    {
        onlyValid = true;
    }
    

    while (scanf("%s", line) == 1)
    {
        S = strtol(line, &end, 10);
        verification(&S, weight, &onlyValid);
    }  

    return 0;
}