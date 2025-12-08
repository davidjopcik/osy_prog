#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main() {

    FILE *f = fopen("test", "w");
    f->_write("äwdw");

    return 0;
}