#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main() {
    int fd[2];
    if(pipe(fd) == -1) {
        perror("pipe error"); 
        return 1;
    };

    const char *msg = "Ahoj";
    size_t len = 4;

    size_t w = write(fd[1], msg, len);
    if (w == -1)
    {
        perror("write");
        return 1;
    }
    
    close(fd[1]);

    char buf[64];
    size_t r = read(fd[0], buf, sizeof(buf));
    if (r == -1)    
    {
        perror("read");
        return 1;
    }
    close(fd[0]);

    printf("precital som: %.*s\n", int(r), buf);

    return 0;
}