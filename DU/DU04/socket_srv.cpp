//***************************************************************************
//
// Program example for labs in subject Operating Systems
//
// Petr Olivka, Dept. of Computer Science, petr.olivka@vsb.cz, 2017
//
// Example of socket server.
//
// This program is example of socket server and it allows to connect and serve
// multiple clients (one thread per client).
// The mandatory argument of program is port number for listening.
//
//***************************************************************************

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
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
#include <ctype.h>
#include <stdbool.h>
#include <pthread.h>
#include <semaphore.h>

#define STR_CLOSE "close"
#define STR_QUIT  "quit"

#define MAX_LINE     10
#define BUFFER_SIZE  10

//***************************************************************************
// log messages

#define LOG_ERROR 0 // errors
#define LOG_INFO  1 // information and notifications
#define LOG_DEBUG 2 // debug messages

int g_debug = LOG_INFO;

void log_msg( int t_log_level, const char *t_form, ... )
{
    const char *out_fmt[] = {
        "ERR: (%d-%s) %s\n",
        "INF: %s\n",
        "DEB: %s\n" };

    if ( t_log_level && t_log_level > g_debug ) return;

    char l_buf[1024];
    va_list l_arg;
    va_start( l_arg, t_form );
    vsprintf( l_buf, t_form, l_arg );
    va_end( l_arg );

    switch ( t_log_level )
    {
        case LOG_INFO:
        case LOG_DEBUG: fprintf( stdout, out_fmt[ t_log_level ], l_buf ); break;
        case LOG_ERROR: fprintf( stderr, out_fmt[ t_log_level ], errno, strerror( errno ), l_buf ); break;
    }
}

//***************************************************************************
// help

void help( int t_narg, char **t_args )
{
    if ( t_narg <= 1 || !strcmp( t_args[1], "-h" ) )
    {
        printf(
            "\n"
            "  Socket server example.\n"
            "\n"
            "  Use: %s [-h -d] port_number\n"
            "\n"
            "    -d  debug mode \n"
            "    -h  this help\n"
            "\n", t_args[0] );

        exit( 0 );
    }

    if ( !strcmp( t_args[1], "-d" ) )
        g_debug = LOG_DEBUG;
}

//***************************************************************************
// shared buffer chránený 3 semaformi

static char g_queue[BUFFER_SIZE][MAX_LINE];
static int  g_q_head = 0;
static int  g_q_tail = 0;

static sem_t g_sem_empty; // voľné miesta
static sem_t g_sem_full;  // obsadené miesta
static sem_t g_sem_mutex; // prístup k bufferu

// single-item API podľa zadania
static void producer_item( const char *item )
{
    sem_wait( &g_sem_empty );
    sem_wait( &g_sem_mutex );

    strncpy( g_queue[g_q_tail], item, MAX_LINE - 1 );
    g_queue[g_q_tail][MAX_LINE - 1] = '\0';
    g_q_tail = ( g_q_tail + 1 ) % BUFFER_SIZE;

    sem_post( &g_sem_mutex );
    sem_post( &g_sem_full );
}

static void consumer_item( char *out )
{
    sem_wait( &g_sem_full );
    sem_wait( &g_sem_mutex );

    strncpy( out, g_queue[g_q_head], MAX_LINE - 1 );
    out[MAX_LINE - 1] = '\0';
    g_q_head = ( g_q_head + 1 ) % BUFFER_SIZE;

    sem_post( &g_sem_mutex );
    sem_post( &g_sem_empty );
}

//***************************************************************************
// čítač aktívnych producentov bez atomic (mutex)

static int g_producer_cnt = 0;
pthread_mutex_t g_cnt_mutex = PTHREAD_MUTEX_INITIALIZER;

//***************************************************************************
// pomocné funkcie pre \n-riadky

static ssize_t recv_line( int fd, char *buf, size_t cap )
{
    size_t i = 0;
    while ( i + 1 < cap )
    {
        char c;
        ssize_t r = recv( fd, &c, 1, 0 );
        if ( r <= 0 ) return 0;
        buf[i++] = c;
        if ( c == '\n' ) break;
    }
    buf[i] = '\0';
    if ( i > 0 && buf[i-1] == '\n' ) buf[i-1] = '\0';
    if ( i > 1 && buf[i-2] == '\r' ) buf[i-2] = '\0';
    return (ssize_t)i;
}

static void send_str( int fd, const char *s )
{
    send( fd, s, (int)strlen( s ), 0 );
}

//***************************************************************************
// dáta pre vlákno

typedef struct {
    int         l_sock_client;
    sockaddr_in l_srv_addr;
    int         cl_num;
} thread_arg_t;

//***************************************************************************
// vláknová obsluha klienta

static void *child_serve( void *parg )
{
    thread_arg_t *a = (thread_arg_t*)parg;
    int l_sock_client    = a->l_sock_client;
    sockaddr_in l_srv_addr = a->l_srv_addr;
    int cl_num           = a->cl_num;
    free( a );

    uint l_lsa = sizeof( l_srv_addr );
    getsockname( l_sock_client, (sockaddr *)&l_srv_addr, &l_lsa );
    log_msg( LOG_INFO, "My IP: '%s'  port: %d", inet_ntoa( l_srv_addr.sin_addr ), ntohs( l_srv_addr.sin_port ) );
    getpeername( l_sock_client, (sockaddr *)&l_srv_addr, &l_lsa );
    log_msg( LOG_INFO, "Client IP: '%s'  port: %d", inet_ntoa( l_srv_addr.sin_addr ), ntohs( l_srv_addr.sin_port ) );

    send_str( l_sock_client, "Task?\n" );

    char line[MAX_LINE];
    if ( !recv_line( l_sock_client, line, sizeof( line ) ) )
    { close( l_sock_client ); return NULL; }

    // PRODUCER
    if ( !strcmp( line, "producer" ) )
    {
        pthread_mutex_lock(&g_cnt_mutex);
        g_producer_cnt++;
        pthread_mutex_unlock(&g_cnt_mutex);

        for (;;)
        {
            if ( !recv_line( l_sock_client, line, sizeof( line ) ) ) break;
            if ( strlen( line ) == 0 ) { send_str( l_sock_client, "OK\n" ); continue; }
            producer_item( line );
            send_str( l_sock_client, "OK\n" );
        }

        pthread_mutex_lock(&g_cnt_mutex);
        g_producer_cnt--;
        pthread_mutex_unlock(&g_cnt_mutex);
    }
    // CONSUMER
    else if ( !strcmp( line, "consumer" ) )
    {
        for (;;)
        {
            pthread_mutex_lock(&g_cnt_mutex);
            int local_cnt = g_producer_cnt;
            pthread_mutex_unlock(&g_cnt_mutex);

            int empty_val = 0;
            sem_getvalue(&g_sem_empty, &empty_val);

            if (local_cnt == 0 && empty_val == BUFFER_SIZE)
                break; // nikto nevyrába a fronta prázdna

            char item[MAX_LINE];
            consumer_item(item);
            char out[MAX_LINE + 4];
            snprintf(out, sizeof(out), "%s\n", item);
            send_str(l_sock_client, out);

            if (!recv_line(l_sock_client, line, sizeof(line))) break;
        }
    }

    log_msg( LOG_DEBUG, "Client [%d] done.", cl_num );
    close( l_sock_client );
    return NULL;
}

//***************************************************************************

int main( int t_narg, char **t_args )
{
    if ( t_narg <= 1 ) help( t_narg, t_args );

    int l_port = 0;

    for ( int i = 1; i < t_narg; i++ )
    {
        if ( !strcmp( t_args[i], "-d" ) ) g_debug = LOG_DEBUG;
        if ( !strcmp( t_args[i], "-h" ) ) help( t_narg, t_args );
        if ( *t_args[i] != '-' && !l_port ) { l_port = atoi( t_args[i] ); break; }
    }

    if ( l_port <= 0 )
    {
        log_msg( LOG_INFO, "Bad or missing port number %d!", l_port );
        help( t_narg, t_args );
    }

    log_msg( LOG_INFO, "Server will listen on port: %d.", l_port );

    // init semaforov
    sem_init( &g_sem_empty, 0, BUFFER_SIZE );
    sem_init( &g_sem_full,  0, 0 );
    sem_init( &g_sem_mutex, 0, 1 );

    int l_sock_listen = socket( AF_INET, SOCK_STREAM, 0 );
    if ( l_sock_listen == -1 ) { log_msg( LOG_ERROR, "Unable to create socket." ); exit(1); }

    in_addr l_addr_any{ INADDR_ANY };
    sockaddr_in l_srv_addr{};
    l_srv_addr.sin_family = AF_INET;
    l_srv_addr.sin_port   = htons( l_port );
    l_srv_addr.sin_addr   = l_addr_any;

    int l_opt = 1;
    setsockopt( l_sock_listen, SOL_SOCKET, SO_REUSEADDR, &l_opt, sizeof( l_opt ) );

    if ( bind( l_sock_listen, (const sockaddr *)&l_srv_addr, sizeof( l_srv_addr ) ) < 0 )
    { log_msg( LOG_ERROR, "Bind failed!" ); close( l_sock_listen ); exit(1); }

    if ( listen( l_sock_listen, 16 ) < 0 )
    { log_msg( LOG_ERROR, "Unable to listen on given port!" ); close( l_sock_listen ); exit(1); }

    log_msg( LOG_INFO, "Enter 'quit' to quit server." );

    int cl_num = 0;

    while ( 1 )
    {
        pollfd l_read_poll[2];
        l_read_poll[0].fd = STDIN_FILENO;  l_read_poll[0].events = POLLIN;
        l_read_poll[1].fd = l_sock_listen; l_read_poll[1].events = POLLIN;

        int l_poll = poll( l_read_poll, 2, -1 );
        if ( l_poll < 0 ) { log_msg( LOG_ERROR, "Function poll failed!" ); exit(1); }

        if ( l_read_poll[0].revents & POLLIN )
        {
            char buf[128];
            int len = read( STDIN_FILENO, buf, sizeof( buf ) );
            if ( len > 0 && !strncmp( buf, STR_QUIT, strlen( STR_QUIT ) ) )
            {
                log_msg( LOG_INFO, "Request to 'quit' entered." );
                close( l_sock_listen );
                break;
            }
        }

        if ( l_read_poll[1].revents & POLLIN )
        {
            cl_num++;
            sockaddr_in l_rsa; socklen_t l_rsa_size = sizeof( l_rsa );
            int l_sock_client = accept( l_sock_listen, (sockaddr *)&l_rsa, &l_rsa_size );
            if ( l_sock_client == -1 ) { log_msg( LOG_ERROR, "Unable to accept new client." ); continue; }

            thread_arg_t *arg = (thread_arg_t*)malloc( sizeof( thread_arg_t ) );
            arg->l_sock_client = l_sock_client;
            arg->l_srv_addr    = l_srv_addr;
            arg->cl_num        = cl_num;

            pthread_t th;
            pthread_create( &th, NULL, child_serve, arg );
            pthread_detach( th );
        }
    }

    close( l_sock_listen );
    return 0;
}