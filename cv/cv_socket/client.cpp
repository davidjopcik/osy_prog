#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

int main() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) { perror("socket"); return 1; }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(9090);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("connect");
        close(sock);
        return 1;
    }

    printf("Pripojený na server!\n");

    // prečítaj správu od servera
    char buf[100];
    ssize_t n = read(sock, buf, sizeof(buf)-1);
    buf[n] = '\0';
    printf("Server: %s", buf);

    // odpíš mu
    const char *msg = "Ahoj server!\n";
    write(sock, msg, strlen(msg));

    close(sock);
    return 0;
}