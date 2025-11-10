/*
 * pthread_helper.c — praktický "one-file" helper pre POSIX vlákna (pthreads)
 *
 * Kompilácia:
 *   gcc -Wall -Wextra -O2 -pthread -o pthread_helper pthread_helper.c
 *
 * Spustenie (ukážkový demo scenár producer-consumer):
 *   ./pthread_helper
 *
 * Čo tu nájdeš:
 *  1) Základ: vytváranie, join/detach, návratové hodnoty, odovzdávanie argumentov
 *  2) Mutex: lock/trylock/timedlock
 *  3) Podmienená premenná: wait/signal/broadcast (producer-consumer)
 *  4) RW lock (read-write lock)
 *  5) Bariéra (barrier)
 *  6) pthread_once (jednorazová inicializácia)
 *  7) TLS (thread-specific data) s cleanupom
 *  8) Zrušenie vlákna (cancel), cleanup-handlers
 *  9) Atribúty vlákna (stack size, detached)
 * 10) Signály a maskovanie v vláknach – stručné minimum
 * 11) Poznámky: memory model, dáta na zásobníku, deadlocky, poradie budenia
 */

#define _GNU_SOURCE
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>

/* ------------------------ Pomocné makrá a utilitky ----------------------- */

#define DIE_IF(cond, msg) do { if (cond) { perror(msg); exit(EXIT_FAILURE); } } while (0)

#define PCHK(call) do { int _e = (call); if (_e) { errno = _e; perror(#call); exit(EXIT_FAILURE); } } while (0)

/* monotónny čas + now + add millis (pre timedlock/wait) */
static void timespec_in_millis_from_now(struct timespec *ts, long ms)
{
    struct timespec now;
#if defined(CLOCK_REALTIME)
    clock_gettime(CLOCK_REALTIME, &now);
#else
    /* fallback */
    struct timeval tv; gettimeofday(&tv, NULL);
    now.tv_sec = tv.tv_sec; now.tv_nsec = (long)tv.tv_usec * 1000L;
#endif
    long sec  = ms / 1000;
    long nsec = (ms % 1000) * 1000000L;
    ts->tv_sec  = now.tv_sec + sec;
    ts->tv_nsec = now.tv_nsec + nsec;
    if (ts->tv_nsec >= 1000000000L) { ts->tv_sec++; ts->tv_nsec -= 1000000000L; }
}

/* drobnosť na spánok v ms */
static void msleep(long ms) { struct timespec ts; ts.tv_sec = ms/1000; ts.tv_nsec = (ms%1000)*1000000L; nanosleep(&ts, NULL); }

/* --------------------- 1) Základné vytvorenie vlákna --------------------- */

struct thread_arg {
    int id;
    const char *text;
};

/* Vlákno: prijme pointer na argument, vráti dynamicky alokovanú hodnotu */
static void *basic_thread_fn(void *arg)
{
    struct thread_arg *a = (struct thread_arg*)arg;
    printf("[basic] thread %d: %s\n", a->id, a->text);
    /* Vrátime výsledok cez pthread_exit / return; v praxi vraciame pointer (napr. malloc) */
    int *result = (int*)malloc(sizeof(int));
    *result = a->id * 10;
    return result; /* ekvivalent pthread_exit(result); */
}

/* Ukážka: create/join + odovzdanie argumentu + návratová hodnota */
static void demo_basic_create_join(void)
{
    pthread_t th;
    struct thread_arg a = { .id = 7, .text = "ahoj z vlákna" };
    PCHK(pthread_create(&th, NULL, basic_thread_fn, &a));
    void *ret = NULL;
    PCHK(pthread_join(th, &ret));
    int *val = (int*)ret;
    printf("[basic] návratová hodnota = %d\n", *val);
    free(val);
}

/* Detached vlákno (bez join) – pozor na životnosť dát! */
static void *detached_fn(void *arg)
{
    (void)arg;
    printf("[detached] bežím a končím…\n");
    return NULL;
}

static void demo_detached(void)
{
    pthread_t th;
    pthread_attr_t at; PCHK(pthread_attr_init(&at));
    PCHK(pthread_attr_setdetachstate(&at, PTHREAD_CREATE_DETACHED));
    PCHK(pthread_create(&th, &at, detached_fn, NULL));
    pthread_attr_destroy(&at);
    /* Nesmieš joinovať detached! */
    msleep(50);
}

/* ---------------------- 2) Mutex: lock/trylock/timed --------------------- */

static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
static int shared_counter = 0;

static void *mutex_worker(void *arg)
{
    (void)arg;
    for (int i = 0; i < 10000; ++i) {
        PCHK(pthread_mutex_lock(&mtx));
        shared_counter++;
        PCHK(pthread_mutex_unlock(&mtx));
    }
    return NULL;
}

static void demo_mutex(void)
{
    pthread_t t1, t2;
    shared_counter = 0;
    PCHK(pthread_create(&t1, NULL, mutex_worker, NULL));
    PCHK(pthread_create(&t2, NULL, mutex_worker, NULL));
    PCHK(pthread_join(t1, NULL));
    PCHK(pthread_join(t2, NULL));
    printf("[mutex] shared_counter = %d (očak. 20000)\n", shared_counter);

    /* trylock */
    if (pthread_mutex_trylock(&mtx) == 0) {
        printf("[mutex] trylock OK\n");
        PCHK(pthread_mutex_unlock(&mtx));
    } else {
        printf("[mutex] trylock BUSY\n");
    }

    /* timedlock (s CLOCK_REALTIME) */
#if defined(_POSIX_TIMEOUTS)
    struct timespec deadline;
    timespec_in_millis_from_now(&deadline, 10);
    int e = pthread_mutex_timedlock(&mtx, &deadline);
    if (!e) {
        printf("[mutex] timedlock OK\n");
        PCHK(pthread_mutex_unlock(&mtx));
    } else if (e == ETIMEDOUT) {
        printf("[mutex] timedlock TIMEOUT\n");
    } else {
        errno = e; perror("pthread_mutex_timedlock");
    }
#endif
}

/* --------- 3) Podmienená premenná (condvar) — producer/consumer ---------- */

#define QSIZE 16
struct queue {
    int buf[QSIZE];
    int head, tail, count;
    pthread_mutex_t m;
    pthread_cond_t  not_empty;
    pthread_cond_t  not_full;
};

static void queue_init(struct queue *q)
{
    q->head = q->tail = q->count = 0;
    PCHK(pthread_mutex_init(&q->m, NULL));
    PCHK(pthread_cond_init(&q->not_empty, NULL));
    PCHK(pthread_cond_init(&q->not_full, NULL));
}

static void queue_destroy(struct queue *q)
{
    pthread_mutex_destroy(&q->m);
    pthread_cond_destroy(&q->not_empty);
    pthread_cond_destroy(&q->not_full);
}

static void queue_push(struct queue *q, int v)
{
    PCHK(pthread_mutex_lock(&q->m));
    while (q->count == QSIZE)
        PCHK(pthread_cond_wait(&q->not_full, &q->m));
    q->buf[q->tail] = v;
    q->tail = (q->tail + 1) % QSIZE;
    q->count++;
    PCHK(pthread_cond_signal(&q->not_empty)); /* alebo broadcast pri viacerých consumeroch */
    PCHK(pthread_mutex_unlock(&q->m));
}

static int queue_pop(struct queue *q)
{
    PCHK(pthread_mutex_lock(&q->m));
    while (q->count == 0)
        PCHK(pthread_cond_wait(&q->not_empty, &q->m));
    int v = q->buf[q->head];
    q->head = (q->head + 1) % QSIZE;
    q->count--;
    PCHK(pthread_cond_signal(&q->not_full));
    PCHK(pthread_mutex_unlock(&q->m));
    return v;
}

struct prodcons_ctx {
    struct queue q;
    int produce_n;
    int consumers;
};

static void *producer_fn(void *arg)
{
    struct prodcons_ctx *c = (struct prodcons_ctx*)arg;
    for (int i = 1; i <= c->produce_n; ++i) {
        queue_push(&c->q, i);
        // simuluj prácu
        // msleep(1);
    }
    /* pošleme koncové značky (počet consumerov) */
    for (int k = 0; k < c->consumers; ++k)
        queue_push(&c->q, -1); /* -1 = poison pill */
    return NULL;
}

static void *consumer_fn(void *arg)
{
    struct prodcons_ctx *c = (struct prodcons_ctx*)arg;
    long sum = 0;
    for (;;) {
        int v = queue_pop(&c->q);
        if (v == -1) break;
        sum += v;
    }
    return (void*)(intptr_t)sum;
}

static void demo_producer_consumer(void)
{
    struct prodcons_ctx ctx;
    queue_init(&ctx.q);
    ctx.produce_n = 100000;
    ctx.consumers = 4;

    pthread_t prod;
    pthread_t cons[4];

    PCHK(pthread_create(&prod, NULL, producer_fn, &ctx));
    for (int i = 0; i < ctx.consumers; ++i)
        PCHK(pthread_create(&cons[i], NULL, consumer_fn, &ctx));

    long total = 0;
    for (int i = 0; i < ctx.consumers; ++i) {
        void *ret = NULL;
        PCHK(pthread_join(cons[i], &ret));
        total += (long)(intptr_t)ret;
    }
    PCHK(pthread_join(prod, NULL));

    /* Súčet 1..N = N*(N+1)/2 */
    long expect = (long)ctx.produce_n * (ctx.produce_n + 1) / 2;
    printf("[condvar] total=%ld, expect=%ld %s\n", total, expect, (total==expect)?"OK":"MISMATCH");

    queue_destroy(&ctx.q);
}

/* -------------------------- 4) Read-Write Lock --------------------------- */

static pthread_rwlock_t rw = PTHREAD_RWLOCK_INITIALIZER;
static int shared_value = 0;

static void *rw_reader(void *arg)
{
    (void)arg;
    for (int i = 0; i < 1000; ++i) {
        PCHK(pthread_rwlock_rdlock(&rw));
        int v = shared_value; /* čítanie paralelne OK */
        (void)v;
        PCHK(pthread_rwlock_unlock(&rw));
    }
    return NULL;
}

static void *rw_writer(void *arg)
{
    (void)arg;
    for (int i = 0; i < 100; ++i) {
        PCHK(pthread_rwlock_wrlock(&rw));
        shared_value++; /* exkluzívne */
        PCHK(pthread_rwlock_unlock(&rw));
    }
    return NULL;
}

static void demo_rwlock(void)
{
    pthread_t r[4], w[2];
    for (int i = 0; i < 4; ++i) PCHK(pthread_create(&r[i], NULL, rw_reader, NULL));
    for (int i = 0; i < 2; ++i) PCHK(pthread_create(&w[i], NULL, rw_writer, NULL));
    for (int i = 0; i < 4; ++i) PCHK(pthread_join(r[i], NULL));
    for (int i = 0; i < 2; ++i) PCHK(pthread_join(w[i], NULL));
    printf("[rwlock] shared_value=%d\n", shared_value);
}

/* ------------------------------- 5) Bariéra ------------------------------- */

#if defined(_POSIX_BARRIERS) && (_POSIX_BARRIERS > 0)
static pthread_barrier_t barrier;

static void *barrier_worker(void *arg)
{
    int id = (int)(intptr_t)arg;
    printf("[barrier] vlákno %d: fáza 1\n", id);
    PCHK(pthread_barrier_wait(&barrier));
    printf("[barrier] vlákno %d: fáza 2\n", id);
    PCHK(pthread_barrier_wait(&barrier));
    return NULL;
}

static void demo_barrier(void)
{
    const int N = 3;
    PCHK(pthread_barrier_init(&barrier, NULL, N));
    pthread_t th[N];
    for (int i = 0; i < N; ++i)
        PCHK(pthread_create(&th[i], NULL, barrier_worker, (void*)(intptr_t)i));
    for (int i = 0; i < N; ++i)
        PCHK(pthread_join(th[i], NULL));
    PCHK(pthread_barrier_destroy(&barrier));
}
#else
static void demo_barrier(void) { printf("[barrier] Nie je podporované na tomto systéme.\n"); }
#endif

/* ---------------------- 6) pthread_once — init raz ----------------------- */

static pthread_once_t once_ctrl = PTHREAD_ONCE_INIT;
static int expensive_global = 0;

static void expensive_init(void)
{
    printf("[once] inicializujem drahý zdroj…\n");
    expensive_global = 42;
}

static void *once_user(void *arg)
{
    (void)arg;
    PCHK(pthread_once(&once_ctrl, expensive_init));
    return NULL;
}

static void demo_once(void)
{
    pthread_t t1, t2, t3;
    PCHK(pthread_create(&t1, NULL, once_user, NULL));
    PCHK(pthread_create(&t2, NULL, once_user, NULL));
    PCHK(pthread_create(&t3, NULL, once_user, NULL));
    PCHK(pthread_join(t1, NULL));
    PCHK(pthread_join(t2, NULL));
    PCHK(pthread_join(t3, NULL));
    printf("[once] expensive_global=%d\n", expensive_global);
}

/* ---------------- 7) TLS (Thread-Specific Data) s cleanupom -------------- */

static pthread_key_t tls_key;

static void tls_destructor(void *ptr)
{
    /* Automaticky sa zavolá pri exite vlákna, ak je TLS nenulové */
    free(ptr);
}

static void *tls_worker(void *arg)
{
    int id = (int)(intptr_t)arg;
    char *slot = (char*)malloc(64);
    snprintf(slot, 64, "Ahoj z TLS, id=%d", id);
    PCHK(pthread_setspecific(tls_key, slot));

    /* hocikde v kóde vlákna môžem získať svoj slot */
    char *again = (char*)pthread_getspecific(tls_key);
    printf("[tls] %s\n", again);
    return NULL; /* cleanup prebehne cez tls_destructor */
}

static void demo_tls(void)
{
    PCHK(pthread_key_create(&tls_key, tls_destructor));
    pthread_t t1, t2;
    PCHK(pthread_create(&t1, NULL, tls_worker, (void*)(intptr_t)1));
    PCHK(pthread_create(&t2, NULL, tls_worker, (void*)(intptr_t)2));
    PCHK(pthread_join(t1, NULL));
    PCHK(pthread_join(t2, NULL));
    PCHK(pthread_key_delete(tls_key));
}

/* ------------------- 8) Zrušenie vlákna + cleanup handler ---------------- */

static void cleanup_pop_free(void *ptr) { free(ptr); }

static void *cancellable_worker(void *arg)
{
    (void)arg;
    /* Povoliť zrušenie v "deferred" režime (default) */
    PCHK(pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL));
    PCHK(pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL));

    char *mem = (char*)malloc(128);
    pthread_cleanup_push(cleanup_pop_free, mem); /* záruka free pri cancel */

    for (int i = 0; i < 1000000; ++i) {
        /* cancellation point – napr. pthread_testcancel(), nanosleep(), read(), etc. */
        if ((i % 10000) == 0) pthread_testcancel();
    }

    pthread_cleanup_pop(1); /* 1 => vykonať cleanup (free) aj na normálny return */
    return NULL;
}

static void demo_cancel(void)
{
    pthread_t th;
    PCHK(pthread_create(&th, NULL, cancellable_worker, NULL));
    msleep(10);
    PCHK(pthread_cancel(th));
    PCHK(pthread_join(th, NULL));
    printf("[cancel] worker zrušený a cleanup prebehol\n");
}

/* -------- 9) Atribúty vlákna: veľkosť zásobníka, detached, policy/hint ---- */

static void demo_attrs(void)
{
    pthread_attr_t at; PCHK(pthread_attr_init(&at));
    /* stack size (napr. 1 MB) — opatrne s príliš malými hodnotami */
    PCHK(pthread_attr_setstacksize(&at, 1<<20));
    /* detached ako v demo_detached(), tu len pre ilustráciu: */
    PCHK(pthread_attr_setdetachstate(&at, PTHREAD_CREATE_JOINABLE));
    /* tvorba/nič ďalšie… */
    pthread_t th; PCHK(pthread_create(&th, &at, detached_fn, NULL));
    PCHK(pthread_join(th, NULL));
    pthread_attr_destroy(&at);
    printf("[attrs] stack-size nastavený, vlákno dobehlo\n");
}

/* ---- 10) Signály a vlákna – minimum: maskovanie signálov v novom vlákne -- */
/*
 * Každé vlákno má vlastnú masku signálov. Typicky:
 *  - V hlavnom vlákne zablokuješ signál(e) a spustíš "signal handling" vlákno,
 *  - to použije sigwait alebo sigwaitinfo a rieši signály synchronne.
 */
static void block_signal(int sig)
{
    sigset_t set; sigemptyset(&set); sigaddsig(&set, sig);
    PCHK(pthread_sigmask(SIG_BLOCK, &set, NULL));
}

static void *signal_waiter(void *arg)
{
    (void)arg;
    sigset_t set; sigemptyset(&set); sigaddsig(&set, SIGUSR1);
    for (;;) {
        int sig = 0;
        int e = sigwait(&set, &sig);
        if (e == 0 && sig == SIGUSR1) {
            printf("[signal] zachytený SIGUSR1\n");
            break;
        }
    }
    return NULL;
}

static void demo_signals(void)
{
    /* zablokuj SIGUSR1 v aktuálnom vlákne (a dediť sa bude do nových) */
    block_signal(SIGUSR1);
    pthread_t th;
    PCHK(pthread_create(&th, NULL, signal_waiter, NULL));
    /* pošli signál procesu (chyti ho waiter) */
    pthread_kill(th, SIGUSR1);
    PCHK(pthread_join(th, NULL));
}

/* ------------------------- 11) Dôležité poznámky --------------------------
 *
 * - Nikdy neodovzdávaj ukazovatele na lokálne dáta (stack) do vlákna, ktoré
 *   môžu skončiť životnosť ešte pred tým, než si ich vlákno prečíta. Použi
 *   buď dynamickú alokáciu (free v threade/cleanup), alebo "dlho žijúce" štruktúry.
 *
 * - Data race = UB. Zdieľané premenne chráň mutexom/rwlockom, alebo použij
 *   C11 atomiky (stdatomic.h) a vhodné memory order.
 *
 * - Condition variable: vždy čakať v while (nie if) a držať zamknutý mutex.
 *   Budenia môžu byť spurious, alebo podmienka už neplatí (iný thread stihol zmeniť stav).
 *
 * - Deadlocky: konzistentné poradie zámkov, vyhýbať sa držaniu zámku počas I/O,
 *   dať pozor na reentranciu a zámky nad tým istým mutexom.
 *
 * - Poradie budenia: POSIX negarantuje férovosť ani presné poradie; ak potrebuješ
 *   fairness, navrhni vlastný protokol (fronty, sequence number, atď.).
 *
 * - join vs detach: každé joinable vlákno MUSÍ byť joinuté, inak leak zdrojov.
 *
 * - Časované operácie: timedwait/timedlock používajú CLOCK_REALTIME (zvyčajne);
 *   ak sa mení systémový čas, môže to ovplyvniť timeouty.
 */

/* ----------------------------- main: ukážky ------------------------------ */

int main(void)
{
    printf("=== PTHREAD HELPER: rychly prehlad API a idiomov ===\n");

    demo_basic_create_join();
    demo_detached();
    demo_mutex();
    demo_producer_consumer();
    demo_rwlock();
    demo_barrier();
    demo_once();
    demo_tls();
    demo_cancel();
    demo_attrs();
    demo_signals();

    printf("=== Hotovo. Kód si vystrihni podla potreby. ===\n");
    return 0;
}