//***************************************************************************
//
// Program example for subject Operating Systems
//
// Petr Olivka, Dept. of Computer Science, petr.olivka@vsb.cz, 2021
//
// Example of socket server/client.
//
// UPRAVA PODLA ZADANIA:
// - klient NEBUDE potrebovat poll()
// - spusta sa: client host port ANIMAL
// - posle: "COMPILE ANIMAL\n" (napr. "COMPILE CAT\n")
// - prijate data ulozi do suboru "<PID>.bin" (PID = getpid())
// - po zavreti socketu zavrie subor
// - klient spravi fork a v potomkovi exec "./<PID>.bin"
// - parent pocka na potomka a skonci
//
//***************************************************************************

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <stdarg.h>
#include <sys/socket.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <sys/wait.h>

#define STR_CLOSE               "close"

//***************************************************************************
// log messages

#define LOG_ERROR               0       // errors
#define LOG_INFO                1       // information and notifications
#define LOG_DEBUG               2       // debug messages

// debug flag
int g_debug = LOG_INFO;

void log_msg( int t_log_level, const char *t_form, ... )
{
    const char *out_fmt[] = {
            "ERR: (%d-%s) %s\n",
            "INF: %s\n",
            "DEB: %s\n" };

    if ( t_log_level && t_log_level > g_debug ) return;

    char l_buf[ 1024 ];
    va_list l_arg;
    va_start( l_arg, t_form );
    vsprintf( l_buf, t_form, l_arg );
    va_end( l_arg );

    switch ( t_log_level )
    {
    case LOG_INFO:
    case LOG_DEBUG:
        fprintf( stdout, out_fmt[ t_log_level ], l_buf );
        break;

    case LOG_ERROR:
        fprintf( stderr, out_fmt[ t_log_level ], errno, strerror( errno ), l_buf );
        break;
    }
}

//***************************************************************************
// help

void help( int t_narg, char **t_args )
{
    if ( t_narg <= 3 || !strcmp( t_args[ 1 ], "-h" ) )
    {
        printf(
            "\n"
            "  Socket client example (no poll).\n"
            "\n"
            "  Use: %s [-h -d] ip_or_name port_number ANIMAL\n"
            "\n"
            "    ANIMAL: CAT | DOG | LION | SNAKE\n"
            "    -d  debug mode \n"
            "    -h  this help\n"
            "\n", t_args[ 0 ] );

        exit( 0 );
    }

    if ( !strcmp( t_args[ 1 ], "-d" ) )
        g_debug = LOG_DEBUG;
}

int is_animal_ok( const char *a )
{
    if ( !a ) return 0;
    if ( !strcmp( a, "CAT" ) ) return 1;
    if ( !strcmp( a, "DOG" ) ) return 1;
    if ( !strcmp( a, "LION" ) ) return 1;
    if ( !strcmp( a, "SNAKE" ) ) return 1;
    return 0;
}

//***************************************************************************

int main( int t_narg, char **t_args )
{
    if ( t_narg <= 3 ) help( t_narg, t_args );

    int l_port = 0;
    char *l_host = NULL;
    char *l_animal = NULL;

    // parsing arguments
    for ( int i = 1; i < t_narg; i++ )
    {
        if ( !strcmp( t_args[ i ], "-d" ) )
            g_debug = LOG_DEBUG;

        if ( !strcmp( t_args[ i ], "-h" ) )
            help( t_narg, t_args );

        if ( *t_args[ i ] != '-' )
        {
            if ( !l_host )
                l_host = t_args[ i ];
            else if ( !l_port )
                l_port = atoi( t_args[ i ] );
            else if ( !l_animal )
                l_animal = t_args[ i ];
        }
    }

    if ( !l_host || !l_port || !l_animal )
    {
        log_msg( LOG_INFO, "Host, port or ANIMAL is missing!" );
        help( t_narg, t_args );
        exit( 1 );
    }

    if ( !is_animal_ok( l_animal ) )
    {
        log_msg( LOG_INFO, "Bad ANIMAL '%s'!", l_animal );
        help( t_narg, t_args );
        exit( 1 );
    }

    log_msg( LOG_INFO, "Connection to '%s':%d. ANIMAL=%s", l_host, l_port, l_animal );

    addrinfo l_ai_req, *l_ai_ans;
    bzero( &l_ai_req, sizeof( l_ai_req ) );
    l_ai_req.ai_family = AF_INET;
    l_ai_req.ai_socktype = SOCK_STREAM;

    int l_get_ai = getaddrinfo( l_host, NULL, &l_ai_req, &l_ai_ans );
    if ( l_get_ai )
    {
        log_msg( LOG_ERROR, "Unknown host name!" );
        exit( 1 );
    }

    sockaddr_in l_cl_addr =  *( sockaddr_in * ) l_ai_ans->ai_addr;
    l_cl_addr.sin_port = htons( l_port );
    freeaddrinfo( l_ai_ans );

    // socket creation
    int l_sock_server = socket( AF_INET, SOCK_STREAM, 0 );
    if ( l_sock_server == -1 )
    {
        log_msg( LOG_ERROR, "Unable to create socket.");
        exit( 1 );
    }

    // connect to server
    if ( connect( l_sock_server, ( sockaddr * ) &l_cl_addr, sizeof( l_cl_addr ) ) < 0 )
    {
        log_msg( LOG_ERROR, "Unable to connect server." );
        close( l_sock_server );
        exit( 1 );
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

    // posli poziadavku
    char l_req[ 128 ];
    snprintf( l_req, sizeof( l_req ), "COMPILE %s\n", l_animal );

    int w = write( l_sock_server, l_req, strlen( l_req ) );
    if ( w < 0 )
    {
        log_msg( LOG_ERROR, "Unable to send request." );
        close( l_sock_server );
        exit( 1 );
    }

    // vytvor subor pre prijate data
    // zadanie pise "PID.bin" -> spravime "<PID>.bin" aby sa to neprepisovalo
    char l_out_name[ 128 ];
    snprintf( l_out_name, sizeof( l_out_name ), "%d.bin", (int) getpid() );

    int l_fd = open( l_out_name, O_CREAT | O_TRUNC | O_WRONLY, 0644 );
    if ( l_fd < 0 )
    {
        log_msg( LOG_ERROR, "Unable to create output file." );
        close( l_sock_server );
        exit( 1 );
    }

    // prijimaj data zo servera a zapisuj do suboru az do EOF (read==0)
    char l_buf[ 4096 ];
    int first = 1;
    while ( 1 )
    {
        int n = read( l_sock_server, l_buf, sizeof( l_buf ) );
        if ( n == 0 )
            break;
        if ( n < 0 )
        {
            log_msg( LOG_ERROR, "Unable to read from server." );
            close( l_fd );
            close( l_sock_server );
            exit( 1 );
        }

        if ( first )
    {
        first = 0;
        if ( n >= (int)strlen("COMPILATION_FAILED") &&
             !strncmp( l_buf, "COMPILATION_FAILED", strlen("COMPILATION_FAILED") ) )
        {
            log_msg( LOG_INFO, "Server: compilation failed." );
            close( l_fd );
            close( l_sock_server );
            unlink( l_out_name );   // zmaz subor
            return 1;
        }
    }

        int off = 0;
        while ( off < n )
        {
            int ww = write( l_fd, l_buf + off, n - off );
            if ( ww <= 0 )
            {
                log_msg( LOG_ERROR, "Unable to write to file." );
                close( l_fd );
                close( l_sock_server );
                exit( 1 );
            }
            off += ww;
        }
    }

    close( l_fd );
    close( l_sock_server );

    // nastav spustitelnost
    chmod( l_out_name, 0755 );

    log_msg( LOG_INFO, "Saved to '%s'. Running...", l_out_name );

    pid_t l_pid = fork();
    if ( l_pid < 0 )
    {
        log_msg( LOG_ERROR, "fork failed" );
        exit( 1 );
    }

    if ( l_pid == 0 )
    {
        // child: exec
        char l_path[ 256 ];
        snprintf( l_path, sizeof( l_path ), "./%s", l_out_name );
        execl( l_path, l_path, (char*) NULL );
perror("execl failed");
        _exit( 127 );
    }

    // parent: wait
    int status = 0;
    waitpid( l_pid, &status, 0 );

    log_msg( LOG_INFO, "Done." );
    return 0;
}