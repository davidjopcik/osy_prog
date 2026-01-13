#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUF 64
#define LINE 1024

static void die(const char *m){
    perror(m);
    exit(1);
}

static int handle_client(int cfd) {
    char buf[BUF];
    char line[LINE];
    int line_len = 0;

    int n;
    while((n = read(cfd, buf, sizeof(buf))) > 0) {
        for(int i = 0; i < n; i++) {
            char c = buf[i];

            if(c == '\n') {
                line[line_len] = '\0';
                dprintf(STDERR_FILENO, "RX: '%s'\n", line);

                if((strcmp(line, "quit")) == 0 ) {
                    write(cfd, "Bye\n", 4);
                    return 0;
                }

                write(cfd, "Ok\n", 3);
                line_len = 0;

            }else {
                line[line_len] = c;
                line_len++;
            }
        }
    }
    return 0;

}


int main(int argc, char **argv) {
    if(argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        return 1;
    }

    int port = atoi(argv[1]);

    int sfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sfd < 0) die("socket");

    int opt = 1;
    setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);

    if(bind(sfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) die("bind");
    if(listen(sfd, 5) < 0) die("listen");

    dprintf(STDERR_FILENO, "Listening on port %d...\n", port);

    while(1) {
        int cfd = accept(sfd, NULL, NULL);
        if (cfd < 0) die("accept");

        pid_t pid = fork();
        if(pid < 0) die("fork");

        if(pid == 0) {
            close(sfd);
            handle_client(cfd);
            close(cfd);
            _exit(0);
        }
        close(cfd);
    }
    

    return 0;
}