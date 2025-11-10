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
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <pthread.h>
#include <stdatomic.h>

#define STR_CLOSE "close"
#define MAX_LINE 256

//***************************************************************************
// log messages

#define LOG_ERROR 0 // errors
#define LOG_INFO 1  // information and notifications
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
    if ( t_narg <= 2 || !strcmp( t_args[1], "-h" ) )
    {
        printf(
            "\n"
            "  Socket client example.\n"
            "\n"
            "  Use: %s [-h -d] ip_or_name port_number role\n"
            "       role: producer | consumer\n"
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
// jednoduché \n IO

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
    send( fd, s, (int)strlen(s), 0 );
}

//***************************************************************************
// globálne pre vlákna

static int g_sock = -1;
static atomic_int g_rate_per_min = 60;

// PRODUCER THREAD: číta jmena.txt, posiela riadky a čaká na OK
// PRODUCER THREAD ...
static void *producer_thread(void *arg)
{
    (void)arg;
    FILE *f = fopen("jmena.txt", "r");
    if (!f) { perror("jmena.txt"); return NULL; }

    char line[MAX_LINE];
    while (fgets(line, sizeof(line), f))
    {
        size_t n = strlen(line);
        while (n && (line[n-1]=='\n' || line[n-1]=='\r')) line[--n] = 0;

        if (strlen(line) == 0) continue;            // preskoč prázdne riadky (istota)

        printf("SEND: %s\n", line); fflush(stdout); // <<< DEBUG TU

        char out[MAX_LINE+4];
        snprintf(out, sizeof(out), "%s\n", line);
        send_str(g_sock, out);

        char ack[MAX_LINE];
        if (!recv_line(g_sock, ack, sizeof(ack))) break;

        int rate = atomic_load(&g_rate_per_min);
        if (rate < 1) rate = 1;
        usleep((useconds_t)((60.0 / rate) * 1000000.0));
    }
    fclose(f);
    return NULL;
}

// CONSUMER THREAD: prijíma mená, vypisuje, odpovedá OK
static void *consumer_thread( void *arg )
{
    (void)arg;
    char line[MAX_LINE];
    for (;;)
    {
        if ( !recv_line( g_sock, line, sizeof(line) ) ) break;
        printf( "RECV: %s\n", line );
        send_str( g_sock, "OK\n" );
        int rate = atomic_load(&g_rate_per_min);
        if (rate < 1) rate = 1;
        usleep((useconds_t)((60.0 / rate) * 1000000.0));

    }
    return NULL;
}

//***************************************************************************

int main( int t_narg, char **t_args )
{
    if ( t_narg <= 2 ) help( t_narg, t_args );

    int l_port = 0;
    char *l_host = nullptr;
    char *l_role = nullptr;

    for ( int i = 1; i < t_narg; i++ )
    {
        if ( !strcmp( t_args[i], "-d" ) ) g_debug = LOG_DEBUG;
        if ( !strcmp( t_args[i], "-h" ) ) help( t_narg, t_args );

        if ( *t_args[i] != '-' )
        {
            if ( !l_host ) l_host = t_args[i];
            else if ( !l_port ) l_port = atoi( t_args[i] );
            else if ( !l_role ) l_role = t_args[i];
        }
    }

    if ( !l_host || !l_port || !l_role )
    {
        log_msg( LOG_INFO, "Host, port or role is missing!" );
        help( t_narg, t_args );
        exit(1);
    }

    log_msg( LOG_INFO, "Connection to '%s':%d. Role: %s", l_host, l_port, l_role );

    addrinfo l_ai_req, *l_ai_ans;
    bzero( &l_ai_req, sizeof( l_ai_req ) );
    l_ai_req.ai_family = AF_INET;
    l_ai_req.ai_socktype = SOCK_STREAM;

    int l_get_ai = getaddrinfo( l_host, nullptr, &l_ai_req, &l_ai_ans );
    if ( l_get_ai ) { log_msg( LOG_ERROR, "Unknown host name!" ); exit(1); }

    sockaddr_in l_cl_addr = *(sockaddr_in *)l_ai_ans->ai_addr;
    l_cl_addr.sin_port = htons( l_port );
    freeaddrinfo( l_ai_ans );

    g_sock = socket( AF_INET, SOCK_STREAM, 0 );
    if ( g_sock == -1 ) { log_msg( LOG_ERROR, "Unable to create socket." ); exit(1); }

    if ( connect( g_sock, (sockaddr *)&l_cl_addr, sizeof( l_cl_addr ) ) < 0 )
    { log_msg( LOG_ERROR, "Unable to connect server." ); exit(1); }

    uint l_lsa = sizeof( l_cl_addr );
    getsockname( g_sock, (sockaddr *)&l_cl_addr, &l_lsa );
    log_msg( LOG_INFO, "My IP: '%s'  port: %d", inet_ntoa( l_cl_addr.sin_addr ), ntohs( l_cl_addr.sin_port ) );
    getpeername( g_sock, (sockaddr *)&l_cl_addr, &l_lsa );
    log_msg( LOG_INFO, "Server IP: '%s'  port: %d", inet_ntoa( l_cl_addr.sin_addr ), ntohs( l_cl_addr.sin_port ) );

    // očakávaj Task?\n
    char line[MAX_LINE];
    recv_line( g_sock, line, sizeof(line) );

    // pošli rolu
    char out[MAX_LINE];
    snprintf( out, sizeof(out), "%s\n", l_role );
    printf("SEND: %s\n", line);
    fflush(stdout);
    send_str( g_sock, out );

    pthread_t th;

    if ( !strcmp( l_role, "producer" ) )
    {
        pthread_create( &th, NULL, producer_thread, NULL );

        // hlavné vlákno: čísla zo stdin menia rýchlosť X/min
        for (;;)
        {
            char buf[64];
            if ( !fgets( buf, sizeof(buf), stdin ) ) break;
            int x = atoi( buf );
            if ( x > 0 ) { atomic_store( &g_rate_per_min, x ); printf( "Rate set to %d per minute\n", x ); }
        }
        pthread_join( th, NULL );
    }
    else if ( !strcmp( l_role, "consumer" ) )
    {
        pthread_create( &th, NULL, consumer_thread, NULL );
        pthread_join( th, NULL );
    }
    else
    {
        log_msg( LOG_INFO, "Unknown role." );
    }

    close( g_sock );
    return 0;
}