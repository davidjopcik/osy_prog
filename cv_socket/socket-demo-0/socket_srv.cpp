//***************************************************************************
//
// Program example for labs in subject Operating Systems
//
// Petr Olivka, Dept. of Computer Science, petr.olivka@vsb.cz, 2017
//
// Example of socket server.
//
// This program is example of socket server and it allows to connect and serve
// more clients. For every new connection the parent process will create
// a child (after accept() fork()) which will handle the communication with
// the client. Parent process only accepts new clients.
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
#include <signal.h>
#include <sys/wait.h>

#define STR_CLOSE   "close"
#define STR_QUIT    "quit"

//***************************************************************************
// log messages

#define LOG_ERROR               0       // errors
#define LOG_INFO                1       // information and notifications
#define LOG_DEBUG               2       // debug messages

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

//***************************************************************************
// function for client handling
// In this function will be implemented the functionality of point 2 of task.
//
//***************************************************************************

void serve_client(int client_sock, int listen_sock)
{
    if (listen_sock >= 0)
        close(listen_sock);

    // --- načítanie rozlíšenia ---
    char res[128];
    int pos = 0, n;
    while (pos < (int)sizeof(res) - 1)
    {
        n = read(client_sock, &res[pos], 1);
        if (n <= 0) break;
        if (res[pos] == '\n') break;
        pos++;
    }
    res[pos] = '\0';
    if (pos == 0)
    {
        close(client_sock);
        exit(0);
    }

    log_msg(LOG_INFO, "Client requested resize: %s", res);

    // --- vytvoríme rouru medzi convert a xz ---
    int pipefd[2];
    if (pipe(pipefd) < 0)
    {
        perror("pipe");
        close(client_sock);
        exit(1);
    }

    pid_t pid_convert = fork();
    if (pid_convert < 0)
    {
        perror("fork convert");
        close(client_sock);
        exit(1);
    }

    if (pid_convert == 0)
    {
        // ==========================
        // 1. DIEŤA – convert
        // ==========================
        close(pipefd[0]);                 // nepoužíva čítaciu stranu
        dup2(pipefd[1], STDOUT_FILENO);   // stdout -> pipe write
        close(pipefd[1]);

        execlp("convert",
               "convert",
               "-resize", res,
               "image.png",
               "-",
               (char *)0);

        perror("execlp convert");
        exit(1);
    }

    pid_t pid_xz = fork();
    if (pid_xz < 0)
    {
        perror("fork xz");
        close(client_sock);
        exit(1);
    }

    if (pid_xz == 0)
    {
        // ==========================
        // 2. DIEŤA – xz kompresor
        // ==========================
        close(pipefd[1]);                 // nepoužíva zápisovú stranu
        dup2(pipefd[0], STDIN_FILENO);    // stdin <- pipe read
        close(pipefd[0]);

        dup2(client_sock, STDOUT_FILENO); // stdout -> socket
        close(client_sock);

        execlp("xz",
               "xz",
               "-z", "--stdout",
               (char *)0);

        perror("execlp xz");
        exit(1);
    }

    // ==========================
    // RODIČ – zavrie nepotrebné fd
    // ==========================
    close(pipefd[0]);
    close(pipefd[1]);
    close(client_sock);

    // počká na obe deti
    int status;
    waitpid(pid_convert, &status, 0);
    waitpid(pid_xz, &status, 0);

    log_msg(LOG_INFO, "Conversion and compression finished.");
    exit(0);
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

    // ignore SIGCHLD to prevent zombies
    signal(SIGCHLD, SIG_IGN);

    log_msg( LOG_INFO, "Server will listen on port: %d.", l_port );

    // socket creation
    int l_sock_listen = socket( AF_INET, SOCK_STREAM, 0 );
    if ( l_sock_listen == -1 )
    {
        log_msg( LOG_ERROR, "Unable to create socket.");
        exit( 1 );
    }

    struct in_addr l_addr_any = { INADDR_ANY };
    struct sockaddr_in l_srv_addr;
    l_srv_addr.sin_family = AF_INET;
    l_srv_addr.sin_port = htons( l_port );
    l_srv_addr.sin_addr = l_addr_any;

    int l_opt = 1;
    if ( setsockopt( l_sock_listen, SOL_SOCKET, SO_REUSEADDR, &l_opt, sizeof( l_opt ) ) < 0 )
        log_msg( LOG_ERROR, "Unable to set socket option!" );

    if ( bind( l_sock_listen, (const struct sockaddr * ) &l_srv_addr, sizeof( l_srv_addr ) ) < 0 )
    {
        log_msg( LOG_ERROR, "Bind failed!" );
        close( l_sock_listen );
        exit( 1 );
    }

    if ( listen( l_sock_listen, 5 ) < 0 )
    {
        log_msg( LOG_ERROR, "Unable to listen on given port!" );
        close( l_sock_listen );
        exit( 1 );
    }

    log_msg( LOG_INFO, "Enter 'quit' to quit server." );

    // --- main loop: accept clients ---
    while ( 1 )
    {
        struct sockaddr_in l_rsa;
        socklen_t l_rsa_size = sizeof( l_rsa );

        int l_sock_client = accept( l_sock_listen, (struct sockaddr *)&l_rsa, &l_rsa_size );
        if (l_sock_client < 0)
        {
            log_msg(LOG_ERROR, "Unable to accept new client.");
            continue;
        }

        // log info
        log_msg(LOG_INFO, "Accepted connection from %s:%d",
                inet_ntoa(l_rsa.sin_addr), ntohs(l_rsa.sin_port));

        // --- fork new process for this client ---
        pid_t pid = fork();
        if (pid < 0)
        {
            perror("fork");
            close(l_sock_client);
            continue;
        }

        if (pid == 0)
        {
            // child: handle client
            serve_client(l_sock_client, l_sock_listen);
        }
        else
        {
            // parent: close client socket and continue accepting
            close(l_sock_client);
        }
    }

    close(l_sock_listen);
    return 0;
}