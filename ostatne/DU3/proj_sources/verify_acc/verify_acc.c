#include "verify.h"

static const int vahy[10] = {1,2,4,8,5,10,9,7,3,6};

int verify(uint64_t x) {
    int sum = 0;
    for (int i = 0; i < 10; i++) {
        int digit = x % 10;
        sum += digit * vahy[i];
        x /= 10;
    }
    return (sum % 11) == 0;
}
