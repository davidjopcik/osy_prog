#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>

int main( int argn, char **argc) {
    stat mystat;
    stat(argc[1], &mystat);
    printf("File %s: size %ld mode %o uid %d\n", argc[1],
        mystat.st_size, mystat.st_mode & 0777, mystat.st_uid
    )
}