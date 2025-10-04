#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "ver.h"

void verification(long S, long *weight, bool valid) {
    long bankNumber = S;
    long sum = 0;
    int i = 0;

    while (bankNumber > 0 && i < 10)  
    {
        sum += (bankNumber % 10) * weight[i];
        bankNumber /= 10;
        i++;
    }

    if ((valid) && (sum % 11 == 0))
    {
        printf("ucet: %010ld: ", S);
        printf("Platny\n");
    }
    if(!(valid))
    {
        printf("ucet: %010ld: ", S);
        printf((sum % 11 == 0) ? "Platny\n" : "Neplatny\n");
    }
}