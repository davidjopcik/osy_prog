// Kompiluj: make
// Spusti:  ./tail_simple subor1 [subor2 ...]
// Stop:    v tom istom adresari urob "touch stop" -> program sa slušne ukončí

#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#define MAX_FILES 64   // jednoduchý limit pre počet súborov

// Vypíše základné info o súbore (režim, čas modifikácie, veľkosť a meno)
void print_file_info(const char *path, struct stat *st) {
    char *t = asctime(localtime(&st->st_mtime)); // obsahuje \n
    if (t && t[strlen(t)-1] == '\n') t[strlen(t)-1] = '\0';
    printf("[INFO] mode=%04o mtime=%s size=%lld name=%s\n",
           (unsigned int)(st->st_mode & 0777),
           t ? t : "(n/a)",
           (long long)st->st_size,
           path);
    fflush(stdout);
}

// Začiatočnícka kontrola: čitateľný bežný súbor (nie adresár) a nie spustiteľný
int is_valid_target(const char *path) {
    struct stat st;
    if (stat(path, &st) == -1) {
        fprintf(stderr, "[SKIP] %s: stat() zlyhal (%s)\n", path, strerror(errno));
        return 0;
    }
    if (S_ISDIR(st.st_mode)) {
        fprintf(stderr, "[SKIP] %s: je adresár\n", path);
        return 0;
    }
    if (access(path, R_OK) != 0) {
        fprintf(stderr, "[SKIP] %s: nečitateľný\n", path);
        return 0;
    }
    if (access(path, X_OK) == 0) {
        fprintf(stderr, "[SKIP] %s: je spustiteľný\n", path);
        return 0;
    }
    return 1;
}

// Kód potomka: sleduje JEDEN súbor, čaká na "check\n" cez ruru
void child_run(const char *path, int pipe_read_end) {
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "[%s] open() zlyhal: %s\n", path, strerror(errno));
        close(pipe_read_end);
        _exit(1);
    }

    struct stat st;
    if (fstat(fd, &st) == -1) {
        fprintf(stderr, "[%s] fstat() zlyhal: %s\n", path, strerror(errno));
        close(fd);
        close(pipe_read_end);
        _exit(1);
    }

    // Úvodný výpis info
    print_file_info(path, &st);

    // Budeme tailovať len NOVÉ prírastky -> začneme od konca
    off_t last = lseek(fd, 0, SEEK_END);
    if (last == (off_t)-1) last = st.st_size;

    // Jednoduchý vstupný buffer. Očakávame presne "check\n"
    char buf[16];

    while (1) {
        ssize_t r = read(pipe_read_end, buf, sizeof(buf));
        if (r == 0) {
            // Rodič zavrel write-end -> koniec
            break;
        }
        if (r < 0) {
            if (errno == EINTR) continue;
            fprintf(stderr, "[%s] read(pipe) zlyhal: %s\n", path, strerror(errno));
            break;
        }

        // Stačí skontrolovať, či prvých 6 znakov je "check\n"
        if (r >= 6 && strncmp(buf, "check\n", 6) == 0) {
            if (fstat(fd, &st) == -1) {
                fprintf(stderr, "[%s] fstat() zlyhal: %s\n", path, strerror(errno));
                break;
            }

            // Ak sa súbor zmenšil (rotácia), začneme od 0
            if (st.st_size < last) {
                last = 0;
            }

            // Nové dáta?
            if (st.st_size > last) {
                print_file_info(path, &st);

                if (lseek(fd, last, SEEK_SET) == (off_t)-1) {
                    fprintf(stderr, "[%s] lseek() zlyhal: %s\n", path, strerror(errno));
                    break;
                }

                off_t to_read = st.st_size - last;
                char chunk[4096];
                while (to_read > 0) {
                    ssize_t want = (to_read > (off_t)sizeof(chunk)) ? (ssize_t)sizeof(chunk) : (ssize_t)to_read;
                    ssize_t n = read(fd, chunk, (size_t)want);
                    if (n <= 0) break;
                    // Vypíš prírastok na stdout
                    ssize_t off = 0;
                    while (off < n) {
                        ssize_t w = write(STDOUT_FILENO, chunk + off, (size_t)(n - off));
                        if (w < 0) { if (errno == EINTR) continue; break; }
                        off += w;
                    }
                    to_read -= n;
                }
                write(STDOUT_FILENO, "\n", 1); // jednoduché oddelenie
                fflush(stdout);
                last = st.st_size;
            }
        }
        // Inak ignorujeme a čakáme na ďalší "check\n"
    }

    close(fd);
    close(pipe_read_end);
    _exit(0);
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr,
            "Pouzitie: %s subor1 [subor2 ...]\n"
            "Priklad:  %s *.log\n", argv[0], argv[0]);
        return 1;
    }

    const char *paths[MAX_FILES];
    int pipes[MAX_FILES][2];   // pipe[i][0]=read pre diet’a, pipe[i][1]=write pre rodica
    pid_t pids[MAX_FILES];
    int count = 0;

    // 1) Prejdi argumenty a vyfiltruj
    printf("=== Overujem vstupy ===\n");
    for (int i = 1; i < argc && count < MAX_FILES; ++i) {
        if (is_valid_target(argv[i])) {
            printf("[OK]  %s\n", argv[i]);
            paths[count++] = argv[i];
        }
    }
    printf("=======================\n");
    fflush(stdout);

    if (count == 0) {
        fprintf(stderr, "Nie je co monitorovat.\n");
        return 1;
    }

    // 2) Na každé dieťa rura + fork
    for (int i = 0; i < count; ++i) {
        if (pipe(pipes[i]) == -1) {
            perror("pipe");
            // jednoduché ukončenie (začiatočnícky štýl)
            return 1;
        }
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            return 1;
        }
        if (pid == 0) {
            // DIEŤA: číta len z read-endu svojej rury
            close(pipes[i][1]); // write-end nepoužívame
            // Dôležité: zavri ostatné rúry zdedené po rodičovi (začiatočnícky jednoduché)
            for (int k = 0; k < i; ++k) {
                close(pipes[k][0]);
                close(pipes[k][1]);
            }
            child_run(paths[i], pipes[i][0]); // nevracia sa
        } else {
            // RODIČ
            pids[i] = pid;
            close(pipes[i][0]); // rodič čítať nebude, iba písať
        }
    }

    // 3) Rodič: každú sekundu pošli "check\n" a sleduj súbor "stop"
    while (1) {
        if (access("stop", F_OK) == 0) {
            // zavri write-endy -> deti uvidia EOF a skončia
            for (int i = 0; i < count; ++i) {
                close(pipes[i][1]);
            }
            break;
        }
        for (int i = 0; i < count; ++i) {
            // jednoduchý protokol: "check\n"
            (void)write(pipes[i][1], "check\n", 6);
        }
        sleep(1);
    }

    // 4) Počkaj na všetky deti
    for (int i = 0; i < count; ++i) {
        int status = 0;
        waitpid(pids[i], &status, 0);
    }

    printf("Hotovo. Zmaž súbor 'stop', aby si mohol program spustiť znova.\n");
    return 0;
}