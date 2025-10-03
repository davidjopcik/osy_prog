#include "gen.h"
#include <stdio.h>
#include <unistd.h>

static void print_padded_u64(unsigned long long x) {
    printf("%010llu\n", x);
}
static void write_bin_u64(unsigned long long x) {
    write(STDOUT_FILENO, &x, sizeof(x));
}

void gen_numbers(uint64_t S, uint64_t N, int binary_mode) {
    for (uint64_t i = 0; i < N; i++) {
        unsigned long long v = S + i;
        if (binary_mode) write_bin_u64(v);
        else             print_padded_u64(v);
    }
}
