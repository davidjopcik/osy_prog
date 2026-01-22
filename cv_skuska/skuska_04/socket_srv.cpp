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
#include <sys/wait.h>
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>

#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

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

sem_t sem[4];

char buf[256];
char file[4096];
int i;
char file_name[128];
int day, month;

void init_sem() {
    for(int i = 1; i < 5; i++) {
        sem_init(&sem[i], 0, 1);
    };
}

void destroy() {
    for(int i = 0; i < 4; i++) {
        sem_destroy(&sem[i]);
    }
}

void pic_map() {
    if(month > 0 && month < 4) {
        i = 1;
    }  
    if(month > 3 && month < 7)  {
        i = 2;
    }  
    if(month > 6 && month < 10) {
        i = 3;
    }  
    if(month > 9 && month < 13)  {
        i = 4;
    }  
    sprintf(file_name, "0%d.png", i);
}

static void sen_data(int scl) {
    int f = open(file_name, O_RDONLY);
    int f_size = lseek(f, 0, SEEK_END);
    lseek(f, 0, SEEK_SET);

    int tout = 3000 *1000/ (f_size / sizeof(file));
    
    log_msg(LOG_INFO, "Posielam data...");
    while(1) {
        int fr = read(f, file, sizeof(file));
        write(scl, file, fr);
        //usleep(1000*1000);
        usleep(tout);
        if(fr <= 0) break;
    }
    log_msg(LOG_INFO, "END...");
    close(f);
}
 

void *client_handle(void *arg) {
    int scl = (int)(intptr_t)arg;
    int n = read(scl, buf, sizeof(buf));
    sscanf(buf, "DAY %d.%d", &day, &month);
    //log_msg(LOG_INFO, "%d\n", day);
    //log_msg(LOG_INFO, "%d\n", month);

    if(day > 31 || day < 0) {
        log_msg(LOG_INFO, "zly den");
        close(scl);
        _exit(0);
    }
    if(month > 12 || month < 0) {
        log_msg(LOG_INFO, "zly mesiac");
        close(scl);
        _exit(0);
    }

    pic_map();

    log_msg(LOG_INFO, "Den a mesiac: %d, %d\n", day, month);
    log_msg(LOG_INFO, "Obrazok: %s\n", file_name);


    sem_wait(&sem[i]);

    sen_data(scl);

    sem_post(&sem[i]);



    return NULL;
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

    log_msg( LOG_INFO, "Enter 'quit' to quit server." );

    // go!
    while ( 1 )
    {
        int l_sock_client = -1;

       

        while ( 1 ) // wait for new client
        {
            
                sockaddr_in l_rsa;
                int l_rsa_size = sizeof( l_rsa );
                // new connection
                l_sock_client = accept( l_sock_listen, ( sockaddr * ) &l_rsa, ( socklen_t * ) &l_rsa_size );
                if ( l_sock_client == -1 )
                {
                    log_msg( LOG_ERROR, "Unable to accept new client." );
                    close( l_sock_listen );
                    exit( 1 );
                }
                uint l_lsa = sizeof( l_srv_addr );
                // my IP
                getsockname( l_sock_client, ( sockaddr * ) &l_srv_addr, &l_lsa );
                log_msg( LOG_INFO, "My IP: '%s'  port: %d",
                                 inet_ntoa( l_srv_addr.sin_addr ), ntohs( l_srv_addr.sin_port ) );
                // client IP
                getpeername( l_sock_client, ( sockaddr * ) &l_srv_addr, &l_lsa );
                log_msg( LOG_INFO, "Client IP: '%s'  port: %d",
                                 inet_ntoa( l_srv_addr.sin_addr ), ntohs( l_srv_addr.sin_port ) );

               
                init_sem();
                pthread_t t;
                pthread_create(&t, NULL, client_handle, (void*)(intptr_t)l_sock_client);
                pthread_join(t, NULL);

                destroy();
                log_msg(LOG_INFO, "Cleaned");
                _exit(0);

            }
        break;
    }   

    return 0;
}
