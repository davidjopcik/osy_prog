
#include "verify.h"
static int count_digits(uint64_t x) {
    int c = 0; do { c++; x/=10; } while (x>0);
    return c;
}

int verify(uint64_t x) {
    int d = count_digits(x);
    if (d == 10) {
        return (x % 11ULL) == 0ULL; // základné pravidlo
    } else if (d == 9) {
        // staršie RČ bez kontrolného mod 11 – akceptujme pre demo
        return 1;
    } else {
        return 0;
    }
}