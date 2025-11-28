//***************************************************************************
//
// Program example for labs in subject Operating Systems
//
// Petr Olivka, Dept. of Computer Science, petr.olivka@vsb.cz, 2017
//
// Producer–consumer server with SHM / MQ, limits, pause/resume, timeout,
// flush, stats and logging.
//
//***************************************************************************

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <stdarg.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <mqueue.h>
#include <time.h>

#define STR_CLOSE   "close"
#define STR_QUIT    "quit"

#define N       8
#define MAX_STR 8192  
#define SEM_MUTEX_NAME  "/pc_mutex"
#define SEM_EMPTY_NAME  "/pc_empty"
#define SEM_FULL_NAME   "/pc_full"
#define SEM_PAUSE_NAME  "/pc_pause"
#define SHM_NAME        "/pc_buffer"
#define MQ_NAME         "/pc_mq"

#define MAX_PRODUCERS 2
#define MAX_CONSUMERS 3

typedef struct
{
    char buffer[N][MAX_STR];
    int head;
    int tail;
    int item_num;

    int total_produced;
    int total_consumed;
    int producer_clients;
    int consumer_clients;
} shm_data;

typedef enum
{
    MODE_SHM,
    MODE_MQ
} queue_mode_t;

static queue_mode_t g_mode = MODE_SHM;

static shm_data *g_shm = NULL;
static int g_shm_fd = -1;

static sem_t *mutex = NULL;
static sem_t *empty = NULL;
static sem_t *full  = NULL;
static sem_t *pause_sem = NULL;

static mqd_t g_mq = (mqd_t)-1;

static volatile sig_atomic_t g_running = 1;

//***************************************************************************
// logovanie

#define LOG_ERROR 0
#define LOG_INFO  1
#define LOG_DEBUG 2

int g_debug = LOG_INFO;
FILE *g_log = NULL;

void log_msg(int t_log_level, const char *t_form, ...)
{
    const char *out_fmt[] = {
        "ERR: (%d-%s) %s\n",
        "INF: %s\n",
        "DEB: %s\n"
    };

    if (t_log_level && t_log_level > g_debug) return;

    char l_buf[1024];
    va_list l_arg;
    va_start(l_arg, t_form);
    vsprintf(l_buf, t_form, l_arg);
    va_end(l_arg);

    FILE *out = NULL;
    if (g_log)
        out = g_log;
    else
        out = (t_log_level == LOG_ERROR) ? stderr : stdout;

    switch (t_log_level)
    {
        case LOG_INFO:
        case LOG_DEBUG:
            fprintf(out, out_fmt[t_log_level], l_buf);
            break;

        case LOG_ERROR:
            fprintf(out, out_fmt[t_log_level], errno, strerror(errno), l_buf);
            break;
    }

    if (g_log)
        fflush(g_log);
}

//***************************************************************************
// help

void help(int t_narg, char **t_args)
{
    if (t_narg <= 1 || !strcmp(t_args[1], "-h"))
    {
        printf(
            "\n"
            "  Socket server example.\n"
            "\n"
            "  Use: %s [-h -d -shm -mq -log file -r] port_number\n"
            "\n"
            "    -d      debug mode\n"
            "    -h      this help\n"
            "    -shm    use shared memory buffer (default)\n"
            "    -mq     use POSIX message queue\n"
            "    -log f  log to file f\n"
            "    -r      remove named IPC objects and exit\n"
            "\n", t_args[0]);
        exit(0);
    }

    if (!strcmp(t_args[1], "-d"))
        g_debug = LOG_DEBUG;

    if (!strcmp(t_args[1], "-r"))
    {
        log_msg(LOG_INFO, "Cleaning named IPC objects.");
        sem_unlink(SEM_MUTEX_NAME);
        sem_unlink(SEM_EMPTY_NAME);
        sem_unlink(SEM_FULL_NAME);
        sem_unlink(SEM_PAUSE_NAME);
        shm_unlink(SHM_NAME);
        mq_unlink(MQ_NAME);
        exit(0);
    }
}

//***************************************************************************
// IPC init

void init_queue(void)
{
    mutex = sem_open(SEM_MUTEX_NAME, O_CREAT, 0666, 1);
    if (mutex == SEM_FAILED)
    {
        log_msg(LOG_ERROR, "sem_open(mutex) failed");
        exit(1);
    }

    empty = sem_open(SEM_EMPTY_NAME, O_CREAT, 0666, N);
    if (empty == SEM_FAILED)
    {
        log_msg(LOG_ERROR, "sem_open(empty) failed");
        exit(1);
    }

    full = sem_open(SEM_FULL_NAME, O_CREAT, 0666, 0);
    if (full == SEM_FAILED)
    {
        log_msg(LOG_ERROR, "sem_open(full) failed");
        exit(1);
    }

    pause_sem = sem_open(SEM_PAUSE_NAME, O_CREAT, 0666, 1);
    if (pause_sem == SEM_FAILED)
    {
        log_msg(LOG_ERROR, "sem_open(pause_sem) failed");
        exit(1);
    }
}

void init_shm(void)
{
    int first = 0;
    g_shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
    if (g_shm_fd < 0)
    {
        log_msg(LOG_INFO, "SHM not found, creating new.");
        g_shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
        if (g_shm_fd < 0)
        {
            log_msg(LOG_ERROR, "shm_open failed");
            exit(1);
        }

        if (ftruncate(g_shm_fd, sizeof(shm_data)) < 0)
        {
            log_msg(LOG_ERROR, "ftruncate failed");
            exit(1);
        }
        first = 1;
    }

    g_shm = (shm_data *)mmap(NULL, sizeof(shm_data),
                             PROT_READ | PROT_WRITE, MAP_SHARED, g_shm_fd, 0);
    if (g_shm == MAP_FAILED)
    {
        log_msg(LOG_ERROR, "mmap failed");
        exit(1);
    }

    if (first)
    {
        sem_unlink(SEM_MUTEX_NAME);
        sem_unlink(SEM_EMPTY_NAME);
        sem_unlink(SEM_FULL_NAME);
        sem_unlink(SEM_PAUSE_NAME);

        memset(g_shm, 0, sizeof(*g_shm));
        g_shm->head = 0;
        g_shm->tail = 0;
        g_shm->item_num = 0;
        g_shm->total_produced = 0;
        g_shm->total_consumed = 0;
        g_shm->producer_clients = 0;
        g_shm->consumer_clients = 0;
        log_msg(LOG_INFO, "SHM initialized (first process).");
    }
    else
    {
        log_msg(LOG_INFO, "SHM attached (existing).");
    }
}

void init_mq(void)
{
    struct mq_attr attr;
    memset(&attr, 0, sizeof(attr));
    attr.mq_maxmsg = N;
    attr.mq_msgsize = MAX_STR;

    g_mq = mq_open(MQ_NAME, O_RDWR);
    if (g_mq == (mqd_t)-1)
    {
        log_msg(LOG_INFO, "MQ not found, creating new.");
        g_mq = mq_open(MQ_NAME, O_CREAT | O_RDWR, 0666, &attr);
        if (g_mq == (mqd_t)-1)
        {
            log_msg(LOG_ERROR, "mq_open failed");
            exit(1);
        }
    }
    else
    {
        log_msg(LOG_INFO, "MQ opened (existing).");
    }
}

void init_ipc(void)
{
    if (g_mode == MODE_MQ)
    {
        init_mq();
    }
    else
    {
        init_shm();
        init_queue();
    }
}

void cleanup_ipc(void)
{
    if (g_mode == MODE_SHM)
    {
        if (mutex)     sem_close(mutex);
        if (empty)     sem_close(empty);
        if (full)      sem_close(full);
        if (pause_sem) sem_close(pause_sem);

        sem_unlink(SEM_MUTEX_NAME);
        sem_unlink(SEM_EMPTY_NAME);
        sem_unlink(SEM_FULL_NAME);
        sem_unlink(SEM_PAUSE_NAME);

        if (g_shm && g_shm != MAP_FAILED)
            munmap(g_shm, sizeof(*g_shm));
        if (g_shm_fd >= 0)
            close(g_shm_fd);
        shm_unlink(SHM_NAME);
    }
    else
    {
        if (g_mq != (mqd_t)-1)
            mq_close(g_mq);
        mq_unlink(MQ_NAME);
    }
}


//***************************************************************************
// producer / consumer

int consumer(char *item)
{
    if (g_mode == MODE_SHM)
    {
        sem_wait(pause_sem);
        sem_post(pause_sem);

        struct timespec ts;
        if (clock_gettime(CLOCK_REALTIME, &ts) == -1)
        {
            log_msg(LOG_ERROR, "clock_gettime failed");
            return -2;
        }
        ts.tv_sec += 5;

        if (sem_timedwait(full, &ts) == -1)
        {
            if (errno == ETIMEDOUT)
            {
                log_msg(LOG_INFO, "Consumer timeout (SHM).");
                item[0] = '\0';
                return -1;
            }
            else
            {
                log_msg(LOG_ERROR, "sem_timedwait(full) failed");
                return -2;
            }
        }

        sem_wait(mutex);
        strncpy(item, g_shm->buffer[g_shm->head], MAX_STR - 1);
        item[MAX_STR - 1] = '\0';
        g_shm->head = (g_shm->head + 1) % N;
        g_shm->item_num--;
        g_shm->total_consumed++;
        sem_post(mutex);
        sem_post(empty);

        return 0;
    }
    else
    {
         struct timespec ts;
        if (clock_gettime(CLOCK_REALTIME, &ts) == -1)
        {
            log_msg(LOG_ERROR, "clock_gettime failed");
            return -2;
        }
        ts.tv_sec += 5;

        unsigned int prio;
        ssize_t n =mq_receive(g_mq, item, MAX_STR, &prio, &ts);
        if (n == -1)
        {
            if (errno == ETIMEDOUT)
            {
                log_msg(LOG_INFO, "Consumer timeout (MQ).");
                item[0] = '\0';
                return -1;
            }
            else
            {
                log_msg(LOG_ERROR, "mq_timedreceive failed");
                item[0] = '\0';
                return -2;
            }
        }

        if (n > 0)
            item[n - 1] = '\0';  
        else
            item[0] = '\0';

        return 0;
    }
}



void producer(char *item)
{
    if (g_mode == MODE_SHM)
    {
        sem_wait(empty);
        sem_wait(mutex);

        g_shm->total_produced++;
        int num = g_shm->total_produced;

        char tmp[MAX_STR];
        //snprintf(tmp, sizeof(tmp), "%d. %s", num, item);
        snprintf(tmp, sizeof(tmp), "%s", item);

        strncpy(g_shm->buffer[g_shm->tail], tmp, MAX_STR - 1);
        g_shm->buffer[g_shm->tail][MAX_STR - 1] = '\0';
        g_shm->tail = (g_shm->tail + 1) % N;
        g_shm->item_num++;

        sem_post(mutex);
        sem_post(full);
    }
    {
        static int mq_counter = 0; 

        mq_counter++;
        char tmp[MAX_STR];
        snprintf(tmp, sizeof(tmp), "%s", item);
        //snprintf(tmp, sizeof(tmp), "%s", mq_counter, item);

        ssize_t len = strnlen(tmp, MAX_STR - 1);
        tmp[len] = '\0';

        /* if (mq_send(g_mq, tmp, len + 1, 0) < 0)
        {
            log_msg(LOG_ERROR, "mq_send failed");
        } */
    }
}

//***************************************************************************
// clients

void run_producer_client(int sock)
{
    char item[MAX_STR];
    char ack[256];
    int local_count = 0;

    while (1)
    {
        int r = read(sock, item, sizeof(item) - 1);
        if (r <= 0)
        {
            log_msg(LOG_INFO, "Producer client disconnected");
            break;
        }
        item[r] = '\0';
        item[strcspn(item, "\r\n")] = '\0';

        if (item[0] == '\0')
            continue;

        producer(item);
        local_count++;

        write(sock, "Ok\n", 3);
    }

    log_msg(LOG_INFO, "Producer client finished, sent %d items.", local_count);
}

void run_consumer_client(int sock)
{
    char item[MAX_STR];
    char ack[256];

    while (1)
    {
        int res = consumer(item);
        if (res == -1)
        {
            // timeout
            write(sock, "Timeout\n", 8);
            log_msg(LOG_INFO, "Consumer client timeout – closing.");
            break;
        }
        else if (res != 0)
        {
            // chyba
            write(sock, "Error\n", 6);
            log_msg(LOG_ERROR, "Consumer internal error – closing.");
            break;
        }

        
        char sendbuf[MAX_STR];
        snprintf(sendbuf, sizeof(sendbuf), "%s\n", item);
        write(sock, sendbuf, strlen(sendbuf));

        int r = read(sock, ack, sizeof(ack) - 1);
        if (r <= 0)
        {
            log_msg(LOG_INFO, "Consumer client disconnected");
            break;
        }
        ack[r] = '\0';
        ack[strcspn(ack, "\r\n")] = '\0';

        if (strcmp(ack, "Ok") != 0)
        {
            log_msg(LOG_INFO, "Consumer sent invalid ACK: '%s'", ack);
        }
    }
}




void *handle_client(void *arg)
{
    int sock = (int)(intptr_t)arg;

    write(sock, "Task?\n", 6);

    char buf[256];
    int r = read(sock, buf, sizeof(buf) - 1);
    if (r <= 0)
    {
        close(sock);
        return NULL;
    }
    buf[r] = '\0';
    buf[strcspn(buf, "\r\n")] = '\0';

    printf("Client choose: %s\n", buf);

    int is_producer = 0;
    int is_consumer = 0;

    if (!strcmp(buf, "producer"))
    {
        if (g_mode == MODE_SHM)
        {
            sem_wait(mutex);
            if (g_shm->producer_clients >= MAX_PRODUCERS)
            {
                sem_post(mutex);
                write(sock, "Too many producers\n", 20);
                close(sock);
                return NULL;
            }
            g_shm->producer_clients++;
            sem_post(mutex);
        }
        is_producer = 1;
        run_producer_client(sock);
    }
    else if (!strcmp(buf, "consumer"))
    {
        if (g_mode == MODE_SHM)
        {
            sem_wait(mutex);
            if (g_shm->consumer_clients >= MAX_CONSUMERS)
            {
                sem_post(mutex);
                write(sock, "Too many consumers\n", 20);
                close(sock);
                return NULL;
            }
            g_shm->consumer_clients++;
            sem_post(mutex);
        }
        is_consumer = 1;
        run_consumer_client(sock);
    }
    else
    {
        write(sock, "Unknown task\n", 13);
    }

    if (g_mode == MODE_SHM && g_shm)
    {
        sem_wait(mutex);
        if (is_producer && g_shm->producer_clients > 0)
            g_shm->producer_clients--;
        if (is_consumer && g_shm->consumer_clients > 0)
            g_shm->consumer_clients--;
        sem_post(mutex);
    }

    close(sock);
    return NULL;
}

//***************************************************************************
// main

int main(int t_narg, char **t_args)
{
    if (t_narg <= 1) help(t_narg, t_args);

    int l_port = 0;
    char *log_file = NULL;

    // parsing arguments
    for (int i = 1; i < t_narg; i++)
    {
        if (!strcmp(t_args[i], "-d"))
            g_debug = LOG_DEBUG;
        else if (!strcmp(t_args[i], "-h"))
            help(t_narg, t_args);
        else if (!strcmp(t_args[i], "-r"))
            help(t_narg, t_args);
        else if (!strcmp(t_args[i], "-shm"))
            g_mode = MODE_SHM;
        else if (!strcmp(t_args[i], "-mq"))
            g_mode = MODE_MQ;
        else if (!strcmp(t_args[i], "-log"))
        {
            if (i + 1 < t_narg)
            {
                log_file = t_args[++i];
            }
        }
        else if (*t_args[i] != '-' && !l_port)
        {
            l_port = atoi(t_args[i]);
        }
    }

    if (log_file)
    {
        g_log = fopen(log_file, "a");
        if (!g_log)
        {
            perror("fopen log_file");
            exit(1);
        }
    }

    if (l_port <= 0)
    {
        log_msg(LOG_INFO, "Bad or missing port number %d!", l_port);
        help(t_narg, t_args);
    }

    log_msg(LOG_INFO, "Server will listen on port: %d.", l_port);

    int l_sock_listen = socket(AF_INET, SOCK_STREAM, 0);
    if (l_sock_listen == -1)
    {
        log_msg(LOG_ERROR, "Unable to create socket.");
        exit(1);
    }

    struct in_addr l_addr_any;
    l_addr_any.s_addr = INADDR_ANY;

    struct sockaddr_in l_srv_addr;
    l_srv_addr.sin_family = AF_INET;
    l_srv_addr.sin_port = htons(l_port);
    l_srv_addr.sin_addr = l_addr_any;

    int l_opt = 1;
    if (setsockopt(l_sock_listen, SOL_SOCKET, SO_REUSEADDR, &l_opt, sizeof(l_opt)) < 0)
        log_msg(LOG_ERROR, "Unable to set socket option!");

    if (bind(l_sock_listen, (const struct sockaddr *)&l_srv_addr, sizeof(l_srv_addr)) < 0)
    {
        log_msg(LOG_ERROR, "Bind failed!");
        close(l_sock_listen);
        exit(1);
    }

    if (listen(l_sock_listen, 10) < 0)
    {
        log_msg(LOG_ERROR, "Unable to listen on given port!");
        close(l_sock_listen);
        exit(1);
    }


    init_ipc();
    signal(SIGCHLD, SIG_IGN);

    while (g_running)
    {
        int l_sock_client = -1;

        struct pollfd l_read_poll[2];
        l_read_poll[0].fd = STDIN_FILENO;
        l_read_poll[0].events = POLLIN;

        l_read_poll[1].fd = l_sock_listen;
        l_read_poll[1].events = POLLIN;

        int l_poll = poll(l_read_poll, 2, -1);
        if (l_poll < 0)
        {
            if (errno == EINTR) continue;
            log_msg(LOG_ERROR, "Function poll failed!");
            break;
        }

        //STDIN
        if (l_read_poll[0].revents & POLLIN)
        {
            char buf[128];
            int l_len = read(STDIN_FILENO, buf, sizeof(buf) - 1);
            if (l_len <= 0)
            {
                log_msg(LOG_INFO, "Stdin closed, stopping server.");
                g_running = 0;
            }
            else
            {
                buf[l_len] = '\0';
                buf[strcspn(buf, "\r\n")] = '\0';

                if (!strcmp(buf, "quit"))
                {
                    log_msg(LOG_INFO, "Request to 'quit' entered.");
                    g_running = 0;
                }
                else if (!strcmp(buf, "-"))
                {
                    if (g_mode == MODE_SHM && pause_sem)
                    {
                        sem_wait(pause_sem);
                        log_msg(LOG_INFO, "Consumers paused.");
                    }
                    else
                    {
                        log_msg(LOG_INFO, "Pause works only in SHM mode.");
                    }
                }
                else if (!strcmp(buf, "+"))
                {
                    if (g_mode == MODE_SHM && pause_sem)
                    {
                        sem_post(pause_sem);
                        log_msg(LOG_INFO, "Consumers resumed.");
                    }
                    else
                    {
                        log_msg(LOG_INFO, "Resume works only in SHM mode.");
                    }
                }
            }
        }

        if (!g_running)
            break;

        // nový klient
        if (l_read_poll[1].revents & POLLIN)
        {
            struct sockaddr_in l_rsa;
            socklen_t l_rsa_size = sizeof(l_rsa);

            l_sock_client = accept(l_sock_listen, (struct sockaddr *)&l_rsa, &l_rsa_size);
            if (l_sock_client == -1)
            {
                log_msg(LOG_ERROR, "Unable to accept new client.");
                continue;
            }

            socklen_t l_lsa = sizeof(l_srv_addr);
            getsockname(l_sock_client, (struct sockaddr *)&l_srv_addr, &l_lsa);
            log_msg(LOG_INFO, "My IP: '%s'  port: %d",
                    inet_ntoa(l_srv_addr.sin_addr), ntohs(l_srv_addr.sin_port));

            getpeername(l_sock_client, (struct sockaddr *)&l_srv_addr, &l_lsa);
            log_msg(LOG_INFO, "Client IP: '%s'  port: %d",
                    inet_ntoa(l_srv_addr.sin_addr), ntohs(l_srv_addr.sin_port));

            pid_t pid = fork();
            if (pid == 0)
            {
                // child
                close(l_sock_listen);
                handle_client((void *)(intptr_t)l_sock_client);

                if (g_mode == MODE_SHM)
                {
                    if (mutex)     sem_close(mutex);
                    if (empty)     sem_close(empty);
                    if (full)      sem_close(full);
                    if (pause_sem) sem_close(pause_sem);
                    if (g_shm)     munmap(g_shm, sizeof(*g_shm));
                    if (g_shm_fd >= 0) close(g_shm_fd);
                }
                else
                {
                    if (g_mq != (mqd_t)-1)
                        mq_close(g_mq);
                }

                if (g_log)
                    fclose(g_log);

                exit(0);
            }
            else if (pid > 0)
            {
                close(l_sock_client);
            }
        }
    }

    close(l_sock_listen);
    cleanup_ipc();

    if (g_log)
        fclose(g_log);

    return 0;
}
