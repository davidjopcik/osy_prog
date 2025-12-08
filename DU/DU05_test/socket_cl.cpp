//***************************************************************************
//
// Program example for subject Operating Systems
//
// Petr Olivka, Dept. of Computer Science, petr.olivka@vsb.cz, 2021
//
// Example of socket server/client.
//
// This program is example of socket client.
// The mandatory arguments of program is IP adress or name of server and
// a port number.
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
#include <netdb.h>
#include <pthread.h>

#define STR_CLOSE   "close"

#define MAX_STR 8192   // max dlzka riadku ~8k

//***************************************************************************
// log messages

#define LOG_ERROR   0
#define LOG_INFO    1
#define LOG_DEBUG   2

int g_debug = LOG_INFO;

void log_msg( int t_log_level, const char *t_form, ... )
{
    const char *out_fmt[] =
    {
        "ERR: (%d-%s) %s\n",
        "INF: %s\n",
        "DEB: %s\n"
    };

    if ( t_log_level && t_log_level > g_debug ) return;

    char l_buf[ 1024 ];
    va_list l_arg;
    va_start( l_arg, t_form );
    vsprintf( l_buf, t_form, l_arg );
    va_end( l_arg );

    if ( t_log_level == LOG_ERROR )
        fprintf( stderr, out_fmt[ t_log_level ], errno, strerror(errno), l_buf );
    else
        fprintf( stdout, out_fmt[ t_log_level ], l_buf );
}

//***************************************************************************
// help

void help( int t_narg, char **t_args )
{
    if ( t_narg <= 1 || !strcmp(t_args[1], "-h") )
    {
        printf(
            "\n"
            "  Socket client example.\n"
            "\n"
            "  Use: %s [-h -d] ip_or_name port_number [file_name]\n"
            "\n"
            "    -d  debug mode\n"
            "    -h  this help\n"
            "  file_name  \n"
            "            \n"
            "\n", t_args[0] );
        exit(0);
    }

    if ( !strcmp(t_args[1], "-d") )
        g_debug = LOG_DEBUG;
}

static int g_names_per_min = 60;   

static char *g_file_name = NULL;
static int   g_file_exists = 0;
static FILE *g_out_file = NULL;

//***************************************************************************
// PRODUCER thread 

void *producer_thread(void *arg)
{
    int sock = (int)(intptr_t)arg;

    //const char *fname = g_file_name ? g_file_name : "jmena.txt";
    const char *fname = g_file_name ? g_file_name : "/home/fei/jop0015/osy/osy_prog/DU05_test/podzim.pbm";
    FILE *f = fopen(fname, "r");
    if (!f)
    {
        log_msg(LOG_ERROR, "Unable to open input file '%s'", fname);
        return NULL;
    }

    char line[MAX_STR];

    while (fgets(line, sizeof(line), f))
    {
        line[strcspn(line, "\r\n")] = '\0';
        if (line[0] == '\0') continue;

        char sendbuf[MAX_STR];
        snprintf(sendbuf, sizeof(sendbuf), "%s\n", line);

        if (write(sock, sendbuf, strlen(sendbuf)) <= 0)
        {
            log_msg(LOG_INFO, "Producer: write to server failed, ending.");
            break;
        }

        char ack[256];
        int r = read(sock, ack, sizeof(ack)-1);
        if (r <= 0)
        {
            log_msg(LOG_INFO, "Producer: server closed connection.");
            break;
        }
        ack[r] = '\0';

        int rate = g_names_per_min;
        if (rate <= 0) rate = 1;
        double sec_per_name = 60.0 / (double)rate;
        useconds_t delay_us = (useconds_t)(sec_per_name * 1000000.0);
        usleep(delay_us);
    }

    fclose(f);
    log_msg(LOG_INFO, "Producer thread finished (EOF in input file).");
    return NULL;
}

//***************************************************************************
// CONSUMER thread 

void *consumer_thread(void *arg)
{
    int sock = (int)(intptr_t)arg;
    char line[MAX_STR];

    while (1)
    {
        int rr = read(sock, line, sizeof(line));
        if (rr <= 0)
        {
            log_msg(LOG_INFO, "Consumer: server closed connection.");
            break;
        }

        write(STDOUT_FILENO, line, rr);

        if (g_out_file)
        {
            size_t w = fwrite(line, 1, rr, g_out_file);
            if (w < (size_t)rr)
            {
                log_msg(LOG_ERROR, "Writing to output file failed");
            }
            fflush(g_out_file);
        }

        if (write(sock, "Ok\n", 3) <= 0)
        {
            log_msg(LOG_INFO, "Consumer: write Ok failed.");
            break;
        }
    }

    log_msg(LOG_INFO, "Consumer thread finished.");
    return NULL;
}

//***************************************************************************
// main

int main( int t_narg, char **t_args )
{
    if ( t_narg <= 2 ) help( t_narg, t_args );

    int l_port = 0;
    char *l_host = NULL;
    char *file_name = NULL;

    // parsing arguments
    for ( int i = 1; i < t_narg; i++ )
    {
        if ( !strcmp(t_args[i], "-d") )
            g_debug = LOG_DEBUG;

        if ( !strcmp(t_args[i], "-h") )
            help(t_narg, t_args);

        if ( *t_args[i] != '-' )
        {
            if ( !l_host )
                l_host = t_args[i];
            else if ( !l_port )
                l_port = atoi(t_args[i]);
            else
                file_name = t_args[i]; 
        }
    }

    if ( !l_host || !l_port )
    {
        log_msg(LOG_INFO, "Host or port is missing!");
        help(t_narg, t_args);
    }

    g_file_name = file_name;
    if (g_file_name)
    {
        if (access(g_file_name, F_OK) == 0)
        {
            g_file_exists = 1;
            log_msg(LOG_INFO, "File '%s' exists, client will send it (producer).", g_file_name);
        }
        else
        {
            g_file_exists = 0;
            log_msg(LOG_INFO, "File '%s' not found, client will receive data into it (consumer).", g_file_name);
        }
    }

    log_msg(LOG_INFO, "Connection to '%s':%d.", l_host, l_port);

    addrinfo l_ai_req, *l_ai_ans;
    bzero(&l_ai_req, sizeof(l_ai_req));
    l_ai_req.ai_family = AF_INET;
    l_ai_req.ai_socktype = SOCK_STREAM;

    if ( getaddrinfo(l_host, NULL, &l_ai_req, &l_ai_ans) )
    {
        log_msg(LOG_ERROR, "Unknown host name!");
        exit(1);
    }

    sockaddr_in l_cl_addr = *(sockaddr_in*)l_ai_ans->ai_addr;
    l_cl_addr.sin_port = htons(l_port);
    freeaddrinfo(l_ai_ans);

    // socket creation
    int l_sock_server = socket(AF_INET, SOCK_STREAM, 0);
    if ( l_sock_server == -1 )
    {
        log_msg(LOG_ERROR, "Unable to create socket.");
        exit(1);
    }

    // connect to server
    if ( connect(l_sock_server, (sockaddr*)&l_cl_addr, sizeof(l_cl_addr)) < 0 )
    {
        log_msg(LOG_ERROR, "Unable to connect server.");
        exit(1);
    }

    uint l_lsa = sizeof( l_cl_addr );
    // my IP
    getsockname( l_sock_server, ( sockaddr * ) &l_cl_addr, &l_lsa );
    log_msg( LOG_INFO, "My IP: '%s'  port: %d",
             inet_ntoa( l_cl_addr.sin_addr ), ntohs( l_cl_addr.sin_port ) );
    // server IP
    getpeername( l_sock_server, ( sockaddr * ) &l_cl_addr, &l_lsa );
    log_msg( LOG_INFO, "Server IP: '%s'  port: %d",
             inet_ntoa( l_cl_addr.sin_addr ), ntohs( l_cl_addr.sin_port ) );

    log_msg( LOG_INFO, "Enter 'close' to close application." );

    //READ "Task?" FROM SERVER
    char l_buf[MAX_STR];
    int r = read(l_sock_server, l_buf, sizeof(l_buf)-1);
    if (r <= 0) { log_msg(LOG_ERROR, "Server closed."); exit(1); }
    l_buf[r] = '\0';

    write(STDOUT_FILENO, l_buf, r);

    int len = 0;

    // role select

    if (g_file_name)
    {
        if (g_file_exists)
            strcpy(l_buf, "producer");
        else
            strcpy(l_buf, "consumer");

        len = strlen(l_buf);
        l_buf[len++] = '\n';
        if (write(l_sock_server, l_buf, len) <= 0)
        {
            log_msg(LOG_ERROR, "Write role to server failed.");
            close(l_sock_server);
            return 1;
        }
        l_buf[len-1] = '\0'; 
    }
    else
    {
        len = read(STDIN_FILENO, l_buf, sizeof(l_buf)-1);
        if (len <= 0) exit(1);
        l_buf[len] = '\0';
        write(l_sock_server, l_buf, len);
        l_buf[strcspn(l_buf, "\r\n")] = '\0';
    }

    // PRODUCER MODE – thread
    if ( !strcmp(l_buf, "producer") )
    {
        log_msg(LOG_INFO, "Entering PRODUCER mode.");

        if (g_file_name && g_file_exists)
        {
            producer_thread((void*)(intptr_t)l_sock_server);
        }
        else
        {
            pthread_t tid;
            pthread_create(&tid, NULL, producer_thread, (void*)(intptr_t)l_sock_server);

            char line[256];
            while (1)
            {
                int n = read(STDIN_FILENO, line, sizeof(line)-1);
                if (n <= 0) break;
                line[n] = '\0';

                int v = atoi(line);
                if (v > 0)
                {
                    g_names_per_min = v;
                    log_msg(LOG_INFO, "New speed: %d names/min", g_names_per_min);
                }
            }

            pthread_cancel(tid); 
            pthread_join(tid, NULL);
        }
    }

    // CONSUMER MODE – thread
    else if ( !strcmp(l_buf, "consumer") )
    {
        log_msg(LOG_INFO, "Entering CONSUMER mode.");

        if (!g_file_exists && g_file_name)
        {
            g_out_file = fopen(g_file_name, "wb");
            if (!g_out_file)
            {
                log_msg(LOG_ERROR, "Unable to create output file '%s'", g_file_name);
            }
        }

        pthread_t tid;
        pthread_create(&tid, NULL, consumer_thread, (void*)(intptr_t)l_sock_server);
        pthread_join(tid, NULL);

        if (g_out_file)
        {
            fclose(g_out_file);
            g_out_file = NULL;
        }
    }

    else
    {
        log_msg(LOG_INFO, "Unknown mode.");
    }

    close(l_sock_server);
    return 0;
}
