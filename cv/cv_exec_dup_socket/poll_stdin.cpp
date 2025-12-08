// file: poll_stdin.c
#include <stdio.h>
#include <unistd.h>
#include <poll.h>
#include <string.h>

int main(void) {
    struct pollfd pfd = { .fd = STDIN_FILENO, .events = POLLIN };
    int ret = poll(&pfd, 1, 10000); // 10 s

    if (ret == -1) { perror("poll"); return 1; }
    if (ret == 0) { printf("⏱️  timeout (nič neprišlo)\n"); return 0; }

    if (pfd.revents & POLLIN) {
        char buf[256];
        ssize_t n = read(STDIN_FILENO, buf, sizeof(buf)-1);
        
        if (n > 0) { 
            if (strcmp(buf, "exit\n") == 0 )
            {
                return 0;
            }
            else
            {
                buf[n] = '\0'; 
                printf("💬 stdin: %s", buf); 
                printf("Pocet znakov: %zu\n", strlen(buf)-1);
            }
                
        }
    }
    return 0;
}