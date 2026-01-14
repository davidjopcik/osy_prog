//***************************************************************************
//
// Program example for labs in subject Operating Systems
//
// Petr Olivka, Dept. of Computer Science, petr.olivka@vsb.cz, 2017
//
// Example of socket server.
//
// UPRAVA PODLA ZADANIA:
// - pre kazdeho klienta novy proces (fork po accept)
// - klient posle prikaz: "COMPILE ANIMAL\n" (napr. "COMPILE CAT\n")
// - server spravi DOWN semaforu (kompiluje sa stale do rovnakeho suboru "animal")
// - server vytvori potomka na kompilaciu (fork + exec g++)
// - parent wait + kontrola WEXITSTATUS(status)==0
// - ak uspech, server posle subor "animal" klientovi pomaly cca 5 sekund
// - server spravi UP semaforu, zavrie socket a proces konci
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
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>
#include <semaphore.h>

#define STR_CLOSE   "close"
#define STR_QUIT    "quit"

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
    if ( t_narg <= 1 || !strcmp( t_args[ 1 ], "-h" ) )
    {
        printf(
            "\n"
            "  Socket server example.\n"
            "\n"
            "  Use: %s [-h -d] port_number\n"
            "\n"
            "    -d  debug mode \n"
            "    -h  this help\n"
            "\n", t_args[ 0 ] );

        exit( 0 );
    }

    if ( !strcmp( t_args[ 1 ], "-d" ) )
        g_debug = LOG_DEBUG;
}

#define LINE 1024
#define SEND_BUF 4096

static const char *SEM_NAME = "/osy_compile_sem";

//***************************************************************************
// pomocne funkcie

// 1) Nacita riadok zo socketu po '\n' (bez riesenia chyb)
int read_line_socket( int lsc, char *t_line, int t_max )
{
    int i = 0;
    char c;

    while ( i < t_max - 1 )
    {
        read( lsc, &c, 1 );      // necakame chyby
        if ( c == '\n' ) break;

        t_line[ i ] = c;
        i++;
    }

    t_line[ i ] = '\0';
    return i;
}

// 2) Povoli len 4 zvierata (najjednoduchsie)
int is_animal_ok( const char *a )
{
    if ( !strcmp( a, "CAT" ) ) return 1;
    if ( !strcmp( a, "DOG" ) ) return 1;
    if ( !strcmp( a, "LION" ) ) return 1;
    if ( !strcmp( a, "SNAKE" ) ) return 1;
    return 0;
}

// 3) Skompiluje animal.cpp do suboru "animal" s -D<ANIMAL>
int compile_animal( const char *t_animal )
{
    pid_t pid = fork();

    if ( pid == 0 )
    {
        char def[ 64 ];
        snprintf( def, sizeof( def ), "-D%s", t_animal );

        // (volitelne) presmeruj vystup kompilatora do suboru
        int fd = open( "compile.log", O_CREAT | O_TRUNC | O_WRONLY, 0644 );
        dup2( fd, 1 );
        dup2( fd, 2 );
        close( fd );

        execlp( "g++", "g++", def, "animal.cpp", "-o", "animal", (char*) NULL );
        _exit( 1 ); // sem sa dostane len ked exec zlyha
    }

    int status;
    waitpid( pid, &status, 0 );          // pockaj na kompilator
    return WEXITSTATUS( status );        // 0 = OK, ine = zlyhanie
}

// 4) Posle subor po kusoch tak, aby to trvalo cca t_total_time_us mikrosekund
int send_file_slow( int lsc, const char *t_fname, int t_total_time_us )
{
    int fd = open( t_fname, O_RDONLY );

    struct stat st;
    fstat( fd, &st );

    long long size = st.st_size;
    long long chunks = ( size + SEND_BUF - 1 ) / SEND_BUF;
    if ( chunks < 1 ) chunks = 1;

    int tout = t_total_time_us / chunks;

    char buf[ SEND_BUF ];
    int r;

    while ( ( r = read( fd, buf, sizeof( buf ) ) ) > 0 )
    {
        write( lsc, buf, r );
        usleep( tout );
    }

    close( fd );
    return 0;
}
//***************************************************************************
// klient v samostatnej funkcii (podla zadania)

void client_handle( int lsc, sem_t *t_sem )
{
    char l_line[ LINE ];
    int l_len = read_line_socket( lsc, l_line, sizeof( l_line ) );
    if ( l_len <= 0 )
    {
        log_msg( LOG_DEBUG, "Client closed or read error." );
        close( lsc );
        return;
    }

    log_msg( LOG_INFO, "Request: '%s'", l_line );

    // ocakavame: "COMPILE ANIMAL"
    char l_cmd[ 64 ];
    char l_animal[ 64 ];
    l_cmd[ 0 ] = 0;
    l_animal[ 0 ] = 0;

    // bezpecne jednoduchy parse
    sscanf( l_line, "%63s %63s", l_cmd, l_animal );

    if ( strcmp( l_cmd, "COMPILE" ) != 0 || !is_animal_ok( l_animal ) )
    {
        const char *msg = "ERR\n";
        write( lsc, msg, strlen( msg ) );
        close( lsc );
        return;
    }

    // DOWN semaforu (kriticka sekcia: kompilacia + citanie/posielanie suboru "animal")
    if ( sem_wait( t_sem ) < 0 )
    {
        log_msg( LOG_ERROR, "sem_wait failed" );
        close( lsc );
        return;
    }

    log_msg( LOG_INFO, "Sem DOWN, compiling '%s'...", l_animal );

    int l_comp = compile_animal( l_animal );
    if ( l_comp != 0 )
    {
        log_msg( LOG_INFO, "Compilation failed." );
        const char *msg = "COMPILATION_FAILED\n";
        write( lsc, msg, strlen( msg ) );

        sem_post( t_sem );
        close( lsc );
        return;
    }

    log_msg( LOG_INFO, "Compilation OK, sending file 'animal'..." );

    // posli binarku pomaly cca 5 sekund
    if ( send_file_slow( lsc, "animal", 5000000 ) != 0 )
    {
        log_msg( LOG_INFO, "Sending failed." );
        // aj tak uvolnime semafor
        sem_post( t_sem );
        close( lsc );
        return;
    }

    // UP semaforu
    sem_post( t_sem );
    log_msg( LOG_INFO, "Sem UP, done." );

    close( lsc );
}

//***************************************************************************

int main( int t_narg, char **t_args )
{
    if ( t_narg <= 1 ) help( t_narg, t_args );

    int l_port = 0;

    // parsing arguments
    for ( int i = 1; i < t_narg; i++ )
    {
        if ( !strcmp( t_args[ i ], "-d" ) )
            g_debug = LOG_DEBUG;

        if ( !strcmp( t_args[ i ], "-h" ) )
            help( t_narg, t_args );

        if ( *t_args[ i ] != '-' && !l_port )
        {
            l_port = atoi( t_args[ i ] );
            break;
        }
    }

    if ( l_port <= 0 )
    {
        log_msg( LOG_INFO, "Bad or missing port number %d!", l_port );
        help( t_narg, t_args );
    }

    log_msg( LOG_INFO, "Server will listen on port: %d.", l_port );

    // aby parent nezbieral zombie (jednoducho)
    signal( SIGCHLD, SIG_IGN );

    // otvor / vytvor semafor
    sem_t *l_sem = sem_open( SEM_NAME, O_CREAT, 0666, 1 );
    if ( l_sem == SEM_FAILED )
    {
        log_msg( LOG_ERROR, "sem_open failed" );
        exit( 1 );
    }

    // socket creation
    int l_sock_listen = socket( AF_INET, SOCK_STREAM, 0 );
    if ( l_sock_listen == -1 )
    {
        log_msg( LOG_ERROR, "Unable to create socket.");
        exit( 1 );
    }

    in_addr l_addr_any = { INADDR_ANY };
    sockaddr_in l_srv_addr;
    l_srv_addr.sin_family = AF_INET;
    l_srv_addr.sin_port = htons( l_port );
    l_srv_addr.sin_addr = l_addr_any;

    // Enable the port number reusing
    int l_opt = 1;
    if ( setsockopt( l_sock_listen, SOL_SOCKET, SO_REUSEADDR, &l_opt, sizeof( l_opt ) ) < 0 )
      log_msg( LOG_ERROR, "Unable to set socket option!" );

    // assign port number to socket
    if ( bind( l_sock_listen, (const sockaddr * ) &l_srv_addr, sizeof( l_srv_addr ) ) < 0 )
    {
        log_msg( LOG_ERROR, "Bind failed!" );
        close( l_sock_listen );
        exit( 1 );
    }

    // listenig on set port
    if ( listen( l_sock_listen, 1 ) < 0 )
    {
        log_msg( LOG_ERROR, "Unable to listen on given port!" );
        close( l_sock_listen );
        exit( 1 );
    }

    // go!
    while ( 1 )
    {
        sockaddr_in l_rsa;
        int l_rsa_size = sizeof( l_rsa );
        int l_sock_client = accept( l_sock_listen, ( sockaddr * ) &l_rsa, ( socklen_t * ) &l_rsa_size );

        if ( l_sock_client == -1 )
        {
            log_msg( LOG_ERROR, "Unable to accept new client." );
            close( l_sock_listen );
            exit( 1 );
        }

        uint l_lsa = sizeof( l_srv_addr );
        getsockname( l_sock_client, ( sockaddr * ) &l_srv_addr, &l_lsa );
        log_msg( LOG_INFO, "My IP: '%s'  port: %d",
                 inet_ntoa( l_srv_addr.sin_addr ), ntohs( l_srv_addr.sin_port ) );
        getpeername( l_sock_client, ( sockaddr * ) &l_srv_addr, &l_lsa );
        log_msg( LOG_INFO, "Client IP: '%s'  port: %d",
                 inet_ntoa( l_srv_addr.sin_addr ), ntohs( l_srv_addr.sin_port ) );

        // pre kazdeho klienta novy proces
        pid_t l_pid = fork();
        if ( l_pid > 0 )
        {
            // parent
            close( l_sock_client );
        }
        else if ( l_pid == 0 )
{
    // child
    signal(SIGCHLD, SIG_DFL);   // <-- KRITICKE: aby waitpid v compile_animal fungoval
    close( l_sock_listen );
    client_handle( l_sock_client, l_sem );
    exit( 0 );
}
        else
        {
            log_msg( LOG_ERROR, "fork failed" );
            close( l_sock_client );
        }
    }

    // sem_close + sem_unlink by sa robilo pri "clean exit"
    return 0;
}