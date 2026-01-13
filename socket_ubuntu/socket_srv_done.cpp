//***************************************************************************
//
// Program example for labs in subject Operating Systems
//
// Petr Olivka, Dept. of Computer Science, petr.olivka@vsb.cz, 2017
//
// Example of socket server.
//
// This program is example of socket server and it allows to connect and serve
// the only one client.
// The mandatory argument of program is port number for listening.
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
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <signal.h>
#include <pthread.h>
#include <semaphore.h>

#define STR_CLOSE "close"
#define STR_QUIT "quit"

//***************************************************************************
// log messages

#define LOG_ERROR 0 // errors
#define LOG_INFO 1  // information and notifications
#define LOG_DEBUG 2 // debug messages

static sem_t sem_get_count;
static sem_t sem_get_run;
static sem_t sem_exec_count;
static sem_t sem_exec_run;

static int g_sock_get = -1;
static int g_sock_exec = -1;

static pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;

#define up(x)   sem_post(&(x))
#define down(x) sem_wait(&(x))

#define BUF 8
#define LINE 1024

// debug flag
int g_debug = LOG_INFO;


void log_msg(int t_log_level, const char *t_form, ...)
{
    const char *out_fmt[] = {
        "ERR: (%d-%s) %s\n",
        "INF: %s\n",
        "DEB: %s\n"};

    if (t_log_level && t_log_level > g_debug)
        return;

    char l_buf[1024];
    va_list l_arg;
    va_start(l_arg, t_form);
    vsprintf(l_buf, t_form, l_arg);
    va_end(l_arg);

    switch (t_log_level)
    {
    case LOG_INFO:
    case LOG_DEBUG:
        fprintf(stdout, out_fmt[t_log_level], l_buf);
        break;

    case LOG_ERROR:
        fprintf(stderr, out_fmt[t_log_level], errno, strerror(errno), l_buf);
        break;
    }
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
            "  Use: %s [-h -d] port_number\n"
            "\n"
            "    -d  debug mode \n"
            "    -h  this help\n"
            "\n",
            t_args[0]);

        exit(0);
    }

    if (!strcmp(t_args[1], "-d"))
        g_debug = LOG_DEBUG;
}

static void close_both() {
    pthread_mutex_lock(&g_lock);
    if(g_sock_get >= 0) {close(g_sock_get); g_sock_get = -1;}
    if(g_sock_exec >= 0) {close(g_sock_exec); g_sock_exec = -1;}
    pthread_mutex_unlock(&g_lock);
}

static void init_sem() {
    sem_init(&sem_get_count, 0, 1);
    sem_init(&sem_get_run, 0, 0);
    sem_init(&sem_exec_count, 0, 1);
    sem_init(&sem_exec_run, 0, 0);
}


static void *client_thread(void *arg)
{
    int sc = *(int*)arg;
    free(arg);

    char buf[2049];
    int n = read(sc, buf, 2048);
    if (n <= 0) { close(sc); return NULL; }
    buf[n] = '\0';

    // GET vetva
    if (!strncmp(buf, "GET", 3)) {
        // 1) favicon => zavri
        if (strstr(buf, "favicon")) { close(sc); return NULL; }

        // 2) povol len 1 GET
        if (sem_trywait(&sem_get_count) != 0) { close(sc); return NULL; }

        // 3) zaregistruj GET socket
        pthread_mutex_lock(&g_lock);
        g_sock_get = sc;
        int exec_sock = g_sock_exec;
        pthread_mutex_unlock(&g_lock);

        // 4) signal EXEC "som tu" a čakaj na EXEC
        up(sem_exec_run);
        down(sem_get_run);

        // teraz by mali byť pripojení obaja
        pthread_mutex_lock(&g_lock);
        exec_sock = g_sock_exec;
        pthread_mutex_unlock(&g_lock);

        if (exec_sock >= 0) {
            write(exec_sock, buf, n);   // pošli celý HTTP request EXEC
        }

        // GET už ďalej nič nerobí, len čaká na odpoveď od EXEC (to bude riešiť EXEC thread)
        // tu len uvoľni "slot" pre ďalší GET po skončení
        up(sem_get_count);
        // NEZATVÁRAJ sc hneď, GET musí dostať odpoveď. Nechaj otvorené.
        return NULL;
    }

    // EXEC vetva
    if (strstr(buf, "EXEC")) {
        if (sem_trywait(&sem_exec_count) != 0) { close(sc); return NULL; }

        pthread_mutex_lock(&g_lock);
        g_sock_exec = sc;
        int get_sock = g_sock_get;
        pthread_mutex_unlock(&g_lock);

        up(sem_get_run);
        down(sem_exec_run);

        // teraz preposielaj všetko od EXEC -> GET
        while (1) {
            char out[2048];
            int k = read(sc, out, sizeof(out));
            if (k <= 0) break;

            pthread_mutex_lock(&g_lock);
            get_sock = g_sock_get;
            pthread_mutex_unlock(&g_lock);

            if (get_sock >= 0) write(get_sock, out, k);
        }

        // keď sa EXEC odpojí, zavri oboch
        close_both();
        up(sem_exec_count);
        return NULL;
    }

    // iné => zavri
    close(sc);
    return NULL;

}

void client_handle(int lsc)
{
    char l_line[LINE];
    int line_len = 0;
    int line_no = 1;
    char msg[LINE];
    char l_buf[BUF];

    while (1)
    {
        int n = read(lsc, l_buf, sizeof(l_buf));
        if (n == 0)
            break;
        if (n < 0)
            break;

        for (int i = 0; i < n; i++)
        {
            if (l_buf[i] == '\n')
            {
                l_line[line_len] = '\0';

                int m = snprintf(msg, sizeof(msg), "%d: %s\n", line_no, l_line);
                write(lsc, msg, m);

                line_len = 0;
                line_no++;
            }
            else
            {
                if (line_len < LINE - 1)
                {
                    l_line[line_len++] = l_buf[i];
                }
            }
        };
    }
};

//***************************************************************************

int main(int t_narg, char **t_args)
{
    if (t_narg <= 1)
        help(t_narg, t_args);

    int l_port = 0;

    // parsing arguments
    for (int i = 1; i < t_narg; i++)
    {
        if (!strcmp(t_args[i], "-d"))
            g_debug = LOG_DEBUG;

        if (!strcmp(t_args[i], "-h"))
            help(t_narg, t_args);

        if (*t_args[i] != '-' && !l_port)
        {
            l_port = atoi(t_args[i]);
            break;
        }
    }

    if (l_port <= 0)
    {
        log_msg(LOG_INFO, "Bad or missing port number %d!", l_port);
        help(t_narg, t_args);
    }

    log_msg(LOG_INFO, "Server will listen on port: %d.", l_port);

    // socket creation
    int l_sock_listen = socket(AF_INET, SOCK_STREAM, 0);
    if (l_sock_listen == -1)
    {
        log_msg(LOG_ERROR, "Unable to create socket.");
        exit(1);
    }

    in_addr l_addr_any = {INADDR_ANY};
    sockaddr_in l_srv_addr;
    l_srv_addr.sin_family = AF_INET;
    l_srv_addr.sin_port = htons(l_port);
    l_srv_addr.sin_addr = l_addr_any;

    // Enable the port number reusing
    int l_opt = 1;
    if (setsockopt(l_sock_listen, SOL_SOCKET, SO_REUSEADDR, &l_opt, sizeof(l_opt)) < 0)
        log_msg(LOG_ERROR, "Unable to set socket option!");

    // assign port number to socket
    if (bind(l_sock_listen, (const sockaddr *)&l_srv_addr, sizeof(l_srv_addr)) < 0)
    {
        log_msg(LOG_ERROR, "Bind failed!");
        close(l_sock_listen);
        exit(1);
    }

    // listenig on set port
    if (listen(l_sock_listen, 1) < 0)
    {
        log_msg(LOG_ERROR, "Unable to listen on given port!");
        close(l_sock_listen);
        exit(1);
    }

    log_msg(LOG_INFO, "Enter 'quit' to quit server.");

    // go!
    init_sem();
    while (1)
    {
        sockaddr_in l_rsa;
        int l_rsa_size = sizeof(l_rsa);
        int l_sock_client = accept(l_sock_listen, (sockaddr *)&l_rsa, (socklen_t *)&l_rsa_size);

        if (l_sock_client == -1)
        {
            log_msg(LOG_ERROR, "Unable to accept new client.");
            close(l_sock_listen);
            exit(1);
        }
        uint l_lsa = sizeof(l_srv_addr);
        // my IP
        getsockname(l_sock_client, (sockaddr *)&l_srv_addr, &l_lsa);
        log_msg(LOG_INFO, "My IP: '%s'  port: %d",
                inet_ntoa(l_srv_addr.sin_addr), ntohs(l_srv_addr.sin_port));
        // client IP
        getpeername(l_sock_client, (sockaddr *)&l_srv_addr, &l_lsa);
        log_msg(LOG_INFO, "Client IP: '%s'  port: %d",
                inet_ntoa(l_srv_addr.sin_addr), ntohs(l_srv_addr.sin_port));

        int *p = malloc(sizeof(int));
        *p = l_sock_client; 

        pthread_t tid;
        pthread_create(&tid, NULL, client_thread, p);
        pthread_detach(tid);


        
    } // while ( 1 )

    return 0;
}
