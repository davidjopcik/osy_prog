// file: poll_two_pipes.c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <poll.h>
#include <sys/wait.h>
#include <string.h>

static void child_writer(int write_fd, const char *msg, int delay_sec) {
    usleep(delay_sec);
    write(write_fd, msg, strlen(msg));
}

int main(void) {
    int p1[2], p2[2];
    if (pipe(p1) == -1 || pipe(p2) == -1) { perror("pipe"); return 1; }

    pid_t c1 = fork();
    if (c1 == -1) { perror("fork"); return 1; }
    if (c1 == 0) {
        close(p1[0]); close(p2[0]); close(p2[1]);
        child_writer(p1[1], "Ahoj z 1 - 1s\n", 1000000);
        child_writer(p1[1], "Ahoj z 1 - 2s\n", 2000000);
        close(p1[1]);
        _exit(0);
    }

    pid_t c2 = fork();
    if (c2 == -1) { perror("fork"); return 1; }
    if (c2 == 0) {
        close(p2[0]); close(p1[0]); close(p1[1]);
        child_writer(p2[1], "Ahoj z 2\n", 1500000);
        close(p2[1]);
        _exit(0);
    }

    // Rodič: sleduje čítacie konce oboch rúr
    close(p1[1]); close(p2[1]);
    struct pollfd fds[2] = {
        { .fd = p1[0], .events = POLLIN },
        { .fd = p2[0], .events = POLLIN }
    };

    int open_streams = 2;
    char buf[256];

    while (open_streams > 0) {
        int ret = poll(fds, 2, 3000); // 3s timeout (len pre ukážku)
        if (ret == -1) { perror("poll"); break; }
        if (ret == 0) { printf("…stále čakám…\n"); continue; }

        for (int i = 0; i < 2; i++) {
            if (fds[i].fd == -1) continue;           // už zatvorené
            if (fds[i].revents & POLLIN) {
                ssize_t n = read(fds[i].fd, buf, sizeof(buf)-1);
                if (n > 0) {
                    buf[n] = '\0';
                    printf("[pipe%d] %s", i+1, buf);
                } else if (n == 0) {
                    // EOF – druhá strana zatvorila
                    close(fds[i].fd);
                    fds[i].fd = -1;
                    open_streams--;
                }
            }
            if (fds[i].revents & (POLLHUP | POLLERR)) {
                // HUP/ERR: zatvor a prestaň sledovať
                close(fds[i].fd);
                fds[i].fd = -1;
                open_streams--;
            }
        }
        

    }

    while (wait(NULL) > 0) { /* no-op */ }
    return 0;
}