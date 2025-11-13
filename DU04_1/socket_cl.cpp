//***************************************************************************
//
// Program example for subject Operating Systems
//
// Petr Olivka, Dept. of Computer Science, petr.olivka@vsb.cz, 2021
//
// Socket client – upravený pre producer/consumer úlohu.
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

#define STR_CLOSE   "close"

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
            "  Use: %s [-h -d] ip_or_name port_number\n"
            "\n"
            "    -d  debug mode\n"
            "    -h  this help\n"
            "\n", t_args[0] );
        exit(0);
    }

    if ( !strcmp(t_args[1], "-d") )
        g_debug = LOG_DEBUG;
}

//***************************************************************************
// main

int main( int t_narg, char **t_args )
{
    if ( t_narg <= 2 ) help( t_narg, t_args );

    int l_port = 0;
    char *l_host = NULL;

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
        }
    }

    if ( !l_host || !l_port )
    {
        log_msg(LOG_INFO, "Host or port is missing!");
        help(t_narg, t_args);
    }

    log_msg(LOG_INFO, "Connection to '%s':%d.", l_host, l_port);

    // hostname resolution
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

    // connect
    if ( connect(l_sock_server, (sockaddr*)&l_cl_addr, sizeof(l_cl_addr)) < 0 )
    {
        log_msg(LOG_ERROR, "Unable to connect server.");
        exit(1);
    }

    log_msg(LOG_INFO, "Connected.");

    //***************************************************************************
    // 1) READ "Task?" FROM SERVER
    //***************************************************************************

    char l_buf[256];
    int r = read(l_sock_server, l_buf, sizeof(l_buf)-1);
    if (r <= 0) { log_msg(LOG_ERROR, "Server closed."); exit(1); }
    l_buf[r] = '\0';

    write(STDOUT_FILENO, l_buf, r);

    //***************************************************************************
    // 2) USER CHOOSES ROLE FROM STDIN
    //***************************************************************************

    int len = read(STDIN_FILENO, l_buf, sizeof(l_buf)-1);
    if (len <= 0) exit(1);
    l_buf[len] = '\0';

    // send chosen role
    write(l_sock_server, l_buf, len);

    // trim newline
    l_buf[strcspn(l_buf, "\r\n")] = '\0';

    //***************************************************************************
    // 3) PRODUCER MODE
    //***************************************************************************

    if ( !strcmp(l_buf, "producer") )
    {
        log_msg(LOG_INFO, "Entering PRODUCER mode.");

        while (1)
        {
            char line[256];

            int n = read(STDIN_FILENO, line, sizeof(line)-1);
            if (n <= 0) break;
            line[n] = '\0';

            // send item to server
            write(l_sock_server, line, n);

            // wait for Ok\n
            int rr = read(l_sock_server, line, sizeof(line)-1);
            if (rr <= 0) break;
            line[rr] = '\0';

            write(STDOUT_FILENO, line, rr);
        }
    }

    //***************************************************************************
    // 4) CONSUMER MODE
    //***************************************************************************

    else if ( !strcmp(l_buf, "consumer") )
    {
        log_msg(LOG_INFO, "Entering CONSUMER mode.");

        while (1)
        {
            char line[256];

            int rr = read(l_sock_server, line, sizeof(line)-1);
            if (rr <= 0) break;
            line[rr] = '\0';
            line[strcspn(line, "\r\n")] = '\0';

            // display item from server
            write(STDOUT_FILENO, line, rr);

            // send ACK
            write(l_sock_server, "Ok\n", 3);
        }
    }

    else
    {
        log_msg(LOG_INFO, "Unknown mode.");
    }

    close(l_sock_server);
    return 0;
}