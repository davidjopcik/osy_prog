#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

/* P3 – Dve rúry (full-duplex)  (priprava)

Súbor: p03_full_duplex.cpp
Cieľ: dialóg — rodič položí otázku, dieťa odpovie cez druhú rúru.

Špecifikácia (prečítaj, napíšeme po P2)
	•	pipe fd_p, pipe fd_ch, fork()
	•	Rodič: píše do fd_p[1], číta z fd_ch[0].
	•	Dieťa: číta z fd_p[0], píše do fd_ch[1].
	•	V každom procese zavri nepoužívané konce rúr hneď po forku. */

int main() {
    int fd_p[2];
    int fd_ch[2];
    if(pipe(fd_p) == -1) {perror("pipe_p"); return 1;}
    if(pipe(fd_ch) == -1) {perror("pipe_ch"); return 1;}
    pid_t p = fork();
    if (p == -1) { perror(" fork"); return 1; };

    if (p > 0)
    {
        //parent
        close(fd_p[0]);
        close(fd_ch[1]);
        const char *question = "Ako sa volas?";
        size_t w = write(fd_p[1], question, strlen(question));
        if(w == -1) {perror("write_p"); return 1;}
        close(fd_p[1]);
        wait(NULL);

        char buf[64];
        size_t r = read(fd_ch[0], buf, sizeof(buf));
        if(r == -1) {perror("read_p"); return 1;}
        printf("Rodic dostal odpoved: %.*s\n", (int)r, buf);
        close(fd_ch[0]);
    }

    if (p == 0)
    {
        //child read
        close(fd_ch[0]);
        close(fd_p[1]);
        char buf[64];
        size_t r = read(fd_p[0], buf, sizeof(buf));
        if(r == -1) {perror("read_ch"); exit(0);}
        printf("Dieta dostalo otazku: %.*s\n", (int)r, buf);
        close(fd_p[0]);
        sleep(2);

        //child write
        const char *answer = "Volam sa dieta";
        size_t w = write(fd_ch[1], answer, strlen(answer));
        if(w == -1) {perror("write_ch"); exit(0);}
        close(fd_ch[1]);
        exit(0);
    }

    return 0;
}