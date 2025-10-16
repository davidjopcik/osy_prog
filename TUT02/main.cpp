#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>


int main() {
    struct stat mystat;
    stat("subor.txt", &mystat);
    printf("Velkost: %ld\n", mystat.st_size);
    printf("Pocet odkazov: %ld\n", mystat.st_nlink);
    printf("Typ: %o\n", mystat.st_mode);

    if (access("subor.txt", R_OK) == 0)
    {
        printf("Subor sa da citat\n");
    }
    
    if (access("subor.txt", W_OK) == 0)
    {
        printf("Do suboru sa da zapisovat\n");
    }

    return 0;
}