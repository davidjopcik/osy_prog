#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char *argv[]) {

    char *end;
    int x = atoi(argv[1]);
    //int x = 4;
    printf("%d\n", x);
    write(1, &x, sizeof x);
        printf("\n");
    
    
    //printf("%b", x);
    
    return 0;
}