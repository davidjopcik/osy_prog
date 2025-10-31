#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define LISTEN_PORT 9090
#define MAX_CLIENTS 64
#define BUF_SZ 4096

static ssize_t write_all(int fd, const void *buf, size_t len) {
    const char *p = (const char*)buf;
    size_t left = len;
    while (left > 0) {
        ssize_t n = write(fd, p, left);
        if (n < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        left -= (size_t)n;
        p    += n;
    }
    return (ssize_t)len;
}

int main(void) {
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd == -1) { perror("socket"); return 1; }

    // Reuse port pri reštarte servera
    int yes = 1;
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
        perror("setsockopt"); close(listenfd); return 1;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(LISTEN_PORT);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // loopback

    if (bind(listenfd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("bind"); close(listenfd); return 1;
    }

    if (listen(listenfd, 16) == -1) {
        perror("listen"); close(listenfd); return 1;
    }

    struct pollfd fds[1 + MAX_CLIENTS];
    memset(fds, 0, sizeof(fds));
    for (int i = 0; i < 1 + MAX_CLIENTS; i++) fds[i].fd = -1;

    // index 0 = listening socket
    fds[0].fd = listenfd;
    fds[0].events = POLLIN;

    printf("Echo server beží na 127.0.0.1:%d …\n", LISTEN_PORT);

    char buf[BUF_SZ];

    for (;;) {
        int ret = poll(fds, 1 + MAX_CLIENTS, -1); // čakaj bez timeoutu
        if (ret == -1) {
            if (errno == EINTR) continue;
            perror("poll"); break;
        }

        // 1) Nové pripojenie?
        if (fds[0].revents & POLLIN) {
            struct sockaddr_in cli; socklen_t clen = sizeof(cli);
            int cfd = accept(listenfd, (struct sockaddr*)&cli, &clen);
            if (cfd == -1) {
                perror("accept");
            } else {
                // priraď do voľného slotu
                int placed = 0;
                for (int i = 1; i < 1 + MAX_CLIENTS; i++) {
                    if (fds[i].fd == -1) {
                        fds[i].fd = cfd;
                        fds[i].events = POLLIN;
                        placed = 1;
                        char ip[INET_ADDRSTRLEN];
                        inet_ntop(AF_INET, &cli.sin_addr, ip, sizeof(ip));
                        printf("[+] Klient %s:%d (slot %d)\n", ip, ntohs(cli.sin_port), i);
                        break;
                    }
                }
                if (!placed) {
                    fprintf(stderr, "Plno, zatváram klienta.\n");
                    close(cfd);
                }
            }
        }

        // 2) Dáta/odpojenie od existujúcich klientov
        for (int i = 1; i < 1 + MAX_CLIENTS; i++) {
            if (fds[i].fd == -1) continue;

            short ev = fds[i].revents;
            if (!ev) continue;

            if (ev & POLLIN) {
                ssize_t n = read(fds[i].fd, buf, sizeof(buf));
                if (n > 0) {
                    // echo späť
                    if (write_all(fds[i].fd, buf, (size_t)n) == -1) {
                        perror("write"); close(fds[i].fd); fds[i].fd = -1;
                    }
                } else if (n == 0) {
                    // EOF = klient zatvoril
                    printf("[-] Klient (slot %d) sa odpojil.\n", i);
                    close(fds[i].fd); fds[i].fd = -1;
                } else { // n < 0
                    if (errno == EINTR) continue;
                    perror("read"); close(fds[i].fd); fds[i].fd = -1;
                }
            }
            if (ev & (POLLHUP | POLLERR)) {
                close(fds[i].fd); fds[i].fd = -1;
            }
        }
    }

    close(listenfd);
    return 0;
}