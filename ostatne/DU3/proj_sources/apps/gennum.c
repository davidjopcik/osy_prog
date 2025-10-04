#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include "gen.h"

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Pouzitie: %s S [N] [-b]\n", argv[0]); return 1;
    }
    uint64_t S = strtoull(argv[1], NULL, 10);
    uint64_t N = 1000;
    int binary_mode = 0;
    for (int i=2;i<argc;i++) {
        if (argv[i][0]=='-' && argv[i][1]=='b') binary_mode=1;
        else N = strtoull(argv[i], NULL, 10);
    }
    gen_numbers(S, N, binary_mode);
    return 0;
}
