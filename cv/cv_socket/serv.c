// gcc server_chat_poll.c -o server
// ./server [port]   (default 9090)
// Test: v iných termináloch spúšťaj `nc 127.0.0.1 9090` a píš správy.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <poll.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAX_CLIENTS 64
#define BUF_SZ 4096

static int listenfd = -1;

static void on_sigint(int sig) {
    if (listenfd != -1) close(listenfd);
    _exit(0);
}

static ssize_t write_all(int fd, const void *buf, size_t len) {
    const char *p = (const char *)buf;
    size_t left = len;
    while (left > 0) {
        ssize_t n = write(fd, p, left);
        if (n < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        left -= (size_t)n;
        p += n;
    }
    return (ssize_t)len;
}

static void trim_newline(char *s) {
    size_t n = strlen(s);
    while (n && (s[n-1] == '\n' || s[n-1] == '\r')) { s[--n] = '\0'; }
}

int main(int argc, char **argv) {
    signal(SIGINT, on_sigint);
    signal(SIGTERM, on_sigint);

    int port = (argc > 1) ? atoi(argv[1]) : 9090;

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd == -1) { perror("socket"); return 1; }

    int yes = 1;
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
        perror("setsockopt SO_REUSEADDR");
        close(listenfd); return 1;
    }

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port   = htons((uint16_t)port);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (bind(listenfd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("bind"); close(listenfd); return 1;
    }
    if (listen(listenfd, 32) == -1) {
        perror("listen"); close(listenfd); return 1;
    }

    struct pollfd fds[2 + MAX_CLIENTS];
    for (int i = 0; i < 2 + MAX_CLIENTS; i++) { fds[i].fd = -1; fds[i].events = 0; }

    // fds[0] = listening socket
    fds[0].fd = listenfd; fds[0].events = POLLIN;
    // fds[1] = stdin
    fds[1].fd = STDIN_FILENO; fds[1].events = POLLIN;

    printf("Chat server beží na 127.0.0.1:%d\n", port);
    printf("Príkazy na serveri: 'list', 'quit'\n");

    char buf[BUF_SZ];

    for (;;) {
        int ret = poll(fds, 2 + MAX_CLIENTS, -1);
        if (ret == -1) {
            if (errno == EINTR) continue;
            perror("poll"); break;
        }

        // 1) Nové pripojenia
        if (fds[0].revents & POLLIN) {
            struct sockaddr_in cli; socklen_t clen = sizeof(cli);
            int cfd = accept(listenfd, (struct sockaddr*)&cli, &clen);
            if (cfd == -1) {
                perror("accept");
            } else {
                int placed = 0;
                for (int i = 2; i < 2 + MAX_CLIENTS; i++) {
                    if (fds[i].fd == -1) {
                        fds[i].fd = cfd;
                        fds[i].events = POLLIN;
                        char ip[INET_ADDRSTRLEN];
                        inet_ntop(AF_INET, &cli.sin_addr, ip, sizeof(ip));
                        printf("[+] Klient %s:%d → slot %d\n", ip, ntohs(cli.sin_port), i);
                        const char *welcome = "Welcome! Napíš 'bye' pre odpojenie.\n";
                        write_all(cfd, welcome, strlen(welcome));
                        placed = 1;
                        break;
                    }
                }
                if (!placed) {
                    const char *full = "Server plný, skús neskôr.\n";
                    write_all(cfd, full, strlen(full));
                    close(cfd);
                }
            }
        }

        // 2) STDIN servera (príkazy)
        if (fds[1].fd != -1 && (fds[1].revents & POLLIN)) {
            ssize_t n = read(STDIN_FILENO, buf, sizeof(buf)-1);
            if (n > 0) {
                buf[n] = '\0'; trim_newline(buf);
                if (strcmp(buf, "quit") == 0) {
                    printf("Koniec… zatváram spojenia.\n");
                    for (int i = 2; i < 2 + MAX_CLIENTS; i++) {
                        if (fds[i].fd != -1) close(fds[i].fd);
                    }
                    close(listenfd);
                    return 0;
                } else if (strcmp(buf, "list") == 0) {
                    printf("Aktívne sloty: ");
                    int first = 1;
                    for (int i = 2; i < 2 + MAX_CLIENTS; i++) {
                        if (fds[i].fd != -1) {
                            if (!first) printf(", ");
                            printf("%d", i); first = 0;
                        }
                    }
                    if (first) printf("(žiadne)");
                    printf("\n");
                } else if (*buf) {
                    printf("Neznámy príkaz: %s (použi 'list' alebo 'quit')\n", buf);
                }
            }
        }

        // 3) Spracovanie klientov
        for (int i = 2; i < 2 + MAX_CLIENTS; i++) {
            if (fds[i].fd == -1) continue;
            short ev = fds[i].revents;
            if (!ev) continue;

            if (ev & POLLIN) {
                ssize_t n = read(fds[i].fd, buf, sizeof(buf)-1);
                if (n > 0) {
                    buf[n] = '\0';
                    // spracuj riadky postupne (jednoduché orezanie pre príkazy)
                    char *p = buf, *nl;
                    while ((nl = strchr(p, '\n')) != NULL) {
                        *nl = '\0';
                        char line[BUF_SZ];
                        strncpy(line, p, sizeof(line)-1);
                        line[sizeof(line)-1] = '\0';
                        trim_newline(line);

                        if (strcmp(line, "bye") == 0) {
                            const char *bye = "bye!\n";
                            write_all(fds[i].fd, bye, strlen(bye));
                            close(fds[i].fd); fds[i].fd = -1;
                            printf("[-] Klient slot %d odpojený (bye)\n", i);
                        } else if (*line) {
                            // broadcast ostatným
                            char msg[BUF_SZ];
                            int m = snprintf(msg, sizeof(msg), "[%d] %s\n", i, line);
                            for (int j = 2; j < 2 + MAX_CLIENTS; j++) {
                                if (j == i) continue;
                                if (fds[j].fd != -1) write_all(fds[j].fd, msg, (size_t)m);
                            }
                        }
                        p = nl + 1;
                    }
                } else if (n == 0) {
                    // EOF
                    printf("[-] Klient slot %d sa odpojil.\n", i);
                    close(fds[i].fd); fds[i].fd = -1;
                } else {
                    if (errno == EINTR) continue;
                    perror("read");
                    close(fds[i].fd); fds[i].fd = -1;
                }
            }
            if (ev & (POLLHUP | POLLERR)) {
                close(fds[i].fd); fds[i].fd = -1;
            }
        }
    }

    if (listenfd != -1) close(listenfd);
    return 0;
}