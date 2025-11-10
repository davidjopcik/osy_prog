/*
 * osy_full_cheatsheet.cpp
 *
 * Jeden .cpp súbor, ktorý zhrňuje:
 *  1) POSIX vlákna (pthread) – create/join, arg/return
 *  2) POSIX semafory – ping-pong bez sleepov
 *  3) Kruhový buffer + Producer/Consumer s 3 semaformi (empty, full, mutex) podľa Tanenbauma
 *  4) Súborové API – stat(), lstat(), fstat(), access() s ukážkami
 *
 * Kompilácia (Linux):
 *   g++ -std=c++11 -Wall -Wextra -O2 -pthread osy_full_cheatsheet.cpp -o osy
 *
 * Spustenie:
 *   ./osy demo_threads                 # pthread create/join + argument/return
 *   ./osy demo_pingpong                # semafory: AB ping-pong bez sleepov
 *   ./osy demo_pc [N]                  # producer/consumer (3 semafory), N položiek (default 50)
 *   ./osy demo_stat <path>             # stat(path) – metadáta a typ súboru (sleduje symlink)
 *   ./osy demo_lstat <path>            # lstat(path) – metadáta linku (nesleduje symlink)
 *   ./osy demo_fstat <path>            # fstat(fd) – metadáta už otvoreného súboru
 *   ./osy demo_access <path>           # access(path, F_OK|R_OK|W_OK|X_OK)
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <cstdint>
#include <string>

#include <pthread.h>
#include <semaphore.h>

#include <sys/stat.h>   // stat, lstat, fstat, struct stat
#include <unistd.h>     // access, open, close
#include <fcntl.h>      // open flags
#include <ctime>        // ctime

/* ======================== Pomocné makrá ======================== */

#define DIE_IF(cond, msg) \
    do { if (cond) { fprintf(stderr, "ERR: %s (%d: %s)\n", msg, errno, strerror(errno)); std::exit(1); } } while(0)

#define OK_OR_DIE(expr, msg) \
    do { int _rc = (expr); if (_rc != 0) { errno = _rc; fprintf(stderr, "ERR: %s (%d: %s)\n", msg, errno, strerror(errno)); std::exit(1); } } while(0)

/* ================================================================
 * 1) PTHREADS – ZÁKLAD: create/join + argument/return
 * ================================================================ */

struct BasicArg {
    int id;
    const char* text;
};

void* basic_thread_fn(void* p) {
    BasicArg* a = static_cast<BasicArg*>(p);
    std::printf("[T%d] ahoj, text='%s'\n", a->id, a->text ? a->text : "(null)");
    return (void*)(intptr_t)(a->id * 10); // návratová „hodnota“
}

static void demo_threads_basic() {
    std::puts("== demo_threads ==");
    pthread_t t1, t2;
    BasicArg a1{1, "prve vlakno"};
    BasicArg a2{2, "druhe vlakno"};

    OK_OR_DIE(pthread_create(&t1, nullptr, basic_thread_fn, &a1), "pthread_create t1");
    OK_OR_DIE(pthread_create(&t2, nullptr, basic_thread_fn, &a2), "pthread_create t2");

    void* ret1 = nullptr; void* ret2 = nullptr;
    OK_OR_DIE(pthread_join(t1, &ret1), "pthread_join t1");
    OK_OR_DIE(pthread_join(t2, &ret2), "pthread_join t2");

    std::printf("[MAIN] návraty: t1=%ld, t2=%ld\n",
                (long)(intptr_t)ret1, (long)(intptr_t)ret2);
}

/* ================================================================
 * 2) POSIX SEMAFORY – Ping-Pong bez sleepov (ABAB...)
 * ================================================================ */

static sem_t sa_pingpong, sb_pingpong;

void* ping_fn(void*) {
    for (int i = 0; i < 20; ++i) {
        sem_wait(&sa_pingpong);
        std::fputc('A', stdout);
        std::fflush(stdout);
        sem_post(&sb_pingpong);
    }
    return nullptr;
}
void* pong_fn(void*) {
    for (int i = 0; i < 20; ++i) {
        sem_wait(&sb_pingpong);
        std::fputc('B', stdout);
        std::fflush(stdout);
        sem_post(&sa_pingpong);
    }
    return nullptr;
}

static void demo_pingpong() {
    std::puts("== demo_pingpong ==");
    OK_OR_DIE(sem_init(&sa_pingpong, 0, 1), "sem_init sa");
    OK_OR_DIE(sem_init(&sb_pingpong, 0, 0), "sem_init sb");

    pthread_t A, B;
    OK_OR_DIE(pthread_create(&A, nullptr, ping_fn, nullptr), "pthread_create A");
    OK_OR_DIE(pthread_create(&B, nullptr, pong_fn, nullptr), "pthread_create B");

    pthread_join(A, nullptr);
    pthread_join(B, nullptr);

    OK_OR_DIE(sem_destroy(&sa_pingpong), "sem_destroy sa");
    OK_OR_DIE(sem_destroy(&sb_pingpong), "sem_destroy sb");
    std::fputc('\n', stdout);
}

/* ================================================================
 * 3) KRUHOVÝ BUFFER (FIFO) + 4) PRODUCER/CONSUMER (3 semafory)
 *    Tanenbaum: empty=N, full=0, mutex=1
 * ================================================================ */

namespace pc {

static const int CAP = 8;
static const int STRLEN = 128;

struct Ring {
    char data[CAP][STRLEN];
    int head = 0; // index na čítanie
    int tail = 0; // index na zápis
    int count = 0;
};

static Ring Q;
static sem_t empty_sem;  // voľné sloty
static sem_t full_sem;   // plné sloty
static sem_t mutex_sem;  // binárny semafor (kritická sekcia)

static void rb_init(Ring& r) { r.head = r.tail = r.count = 0; }

static int rb_push(Ring& r, const char* s) {
    if (r.count == CAP) return -1;
    std::strncpy(r.data[r.tail], s ? s : "", STRLEN - 1);
    r.data[r.tail][STRLEN - 1] = '\0';
    r.tail = (r.tail + 1) % CAP;
    ++r.count;
    return 0;
}
static int rb_pop(Ring& r, char* out) {
    if (r.count == 0) return -1;
    std::strncpy(out, r.data[r.head], STRLEN);
    out[STRLEN - 1] = '\0';
    r.head = (r.head + 1) % CAP;
    --r.count;
    return 0;
}

/* Jednopoložkové funkcie bez vlastných while-slučiek */
static void producer(const char* item) {
    sem_wait(&empty_sem);
    sem_wait(&mutex_sem);
    (void)rb_push(Q, item);
    sem_post(&mutex_sem);
    sem_post(&full_sem);
}
static void consumer(char* out) {
    sem_wait(&full_sem);
    sem_wait(&mutex_sem);
    (void)rb_pop(Q, out);
    sem_post(&mutex_sem);
    sem_post(&empty_sem);
}

/* Demo: 1 producent + 1 konzument, každý N položiek */
struct DemoArgs { int N; };

static void* producer_thread(void* p) {
    int N = static_cast<DemoArgs*>(p)->N;
    char msg[64];
    for (int i = 0; i < N; ++i) {
        std::snprintf(msg, sizeof(msg), "msg-%d", i);
        producer(msg);
    }
    return nullptr;
}
static void* consumer_thread(void* p) {
    int N = static_cast<DemoArgs*>(p)->N;
    char out[STRLEN];
    for (int i = 0; i < N; ++i) {
        consumer(out);
        std::printf("[cons] %s\n", out);
    }
    return nullptr;
}

static void demo_pc(int N = 50) {
    std::puts("== demo_pc ==");
    rb_init(Q);
    OK_OR_DIE(sem_init(&empty_sem, 0, CAP), "sem_init empty");
    OK_OR_DIE(sem_init(&full_sem,  0, 0),   "sem_init full");
    OK_OR_DIE(sem_init(&mutex_sem, 0, 1),   "sem_init mutex");

    DemoArgs args{N};
    pthread_t pt, ct;
    OK_OR_DIE(pthread_create(&pt, nullptr, producer_thread, &args), "pthread_create producer");
    OK_OR_DIE(pthread_create(&ct, nullptr, consumer_thread, &args), "pthread_create consumer");

    pthread_join(pt, nullptr);
    pthread_join(ct, nullptr);

    OK_OR_DIE(sem_destroy(&empty_sem), "sem_destroy empty");
    OK_OR_DIE(sem_destroy(&full_sem),  "sem_destroy full");
    OK_OR_DIE(sem_destroy(&mutex_sem), "sem_destroy mutex");
}

} // namespace pc

/* ================================================================
 * 5) SÚBORY – stat(), lstat(), fstat(), access()
 *    Rýchly helper + praktické výpisy
 * ================================================================ */

static const char* file_type_str(mode_t m) {
    if (S_ISREG(m))  return "regular";
    if (S_ISDIR(m))  return "directory";
    if (S_ISLNK(m))  return "symlink";
    if (S_ISCHR(m))  return "char-device";
    if (S_ISBLK(m))  return "block-device";
    if (S_ISFIFO(m)) return "fifo";
    if (S_ISSOCK(m)) return "socket";
    return "unknown";
}

static void print_stat_like(const char* path, const struct stat& st, const char* label) {
    std::printf("== %s(\"%s\") ==\n", label, path);
    std::printf("typ: %-12s | režim (octal): %o | link count: %ld\n",
                file_type_str(st.st_mode), (unsigned)st.st_mode, (long)st.st_nlink);
    std::printf("veľkosť: %lld B | uid: %d | gid: %d\n",
                (long long)st.st_size, (int)st.st_uid, (int)st.st_gid);
    std::printf("mtime: %s", std::ctime(&st.st_mtime)); // obsahuje '\n'
}

static void demo_stat_path(const char* path, bool follow_symlink) {
    struct stat st{};
    int rc = follow_symlink ? ::stat(path, &st) : ::lstat(path, &st);
    if (rc == -1) { perror(follow_symlink ? "stat" : "lstat"); return; }
    print_stat_like(path, st, follow_symlink ? "stat" : "lstat");
}

static void demo_fstat_fd(const char* path) {
    int fd = ::open(path, O_RDONLY);
    if (fd < 0) { perror("open"); return; }
    struct stat st{};
    if (::fstat(fd, &st) == -1) { perror("fstat"); ::close(fd); return; }
    print_stat_like(path, st, "fstat");
    ::close(fd);
}

static void demo_access_path(const char* path) {
    std::printf("== access(\"%s\") ==\n", path);
    std::printf("F_OK (existuje) : %s\n", (::access(path, F_OK)==0) ? "YES" : "NO");
    std::printf("R_OK (čítanie)  : %s\n", (::access(path, R_OK)==0) ? "YES" : "NO");
    std::printf("W_OK (zápis)    : %s\n", (::access(path, W_OK)==0) ? "YES" : "NO");
    std::printf("X_OK (spustenie): %s\n", (::access(path, X_OK)==0) ? "YES" : "NO");
}

/* ================================================================
 * main – výber dema podľa argumentu
 * ================================================================ */

int main(int argc, char** argv) {
    if (argc < 2) {
        std::fprintf(stderr,
            "Použitie:\n"
            "  %s demo_threads                 # pthread create/join + arg/return\n"
            "  %s demo_pingpong                # semafory: AB ping-pong bez sleepov\n"
            "  %s demo_pc [N]                  # producer/consumer (3 semafory), N položiek (default 50)\n"
            "  %s demo_stat <path>             # stat(path)\n"
            "  %s demo_lstat <path>            # lstat(path)\n"
            "  %s demo_fstat <path>            # fstat(path)\n"
            "  %s demo_access <path>           # access(path)\n",
            argv[0], argv[0], argv[0], argv[0], argv[0], argv[0], argv[0]);
        return 2;
    }

    std::string mode = argv[1];

    if (mode == "demo_threads") {
        demo_threads_basic();
        return 0;
    }
    if (mode == "demo_pingpong") {
        demo_pingpong();
        return 0;
    }
    if (mode == "demo_pc") {
        int N = 50;
        if (argc >= 3) {
            long tmp = std::strtol(argv[2], nullptr, 10);
            if (tmp > 0) N = (int)tmp;
        }
        pc::demo_pc(N);
        return 0;
    }
    if (mode == "demo_stat") {
        if (argc < 3) { std::fprintf(stderr, "Chýba path.\n"); return 2; }
        demo_stat_path(argv[2], /*follow_symlink=*/true);
        return 0;
    }
    if (mode == "demo_lstat") {
        if (argc < 3) { std::fprintf(stderr, "Chýba path.\n"); return 2; }
        demo_stat_path(argv[2], /*follow_symlink=*/false);
        return 0;
    }
    if (mode == "demo_fstat") {
        if (argc < 3) { std::fprintf(stderr, "Chýba path.\n"); return 2; }
        demo_fstat_fd(argv[2]);
        return 0;
    }
    if (mode == "demo_access") {
        if (argc < 3) { std::fprintf(stderr, "Chýba path.\n"); return 2; }
        demo_access_path(argv[2]);
        return 0;
    }

    std::fprintf(stderr, "Neznámy režim: %s\n", mode.c_str());
    return 2;
}