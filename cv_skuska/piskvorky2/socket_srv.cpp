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
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <semaphore.h>
#include <sys/mman.h>


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

#define SHM_NAME        "/shm_example"
#define BOARD_SIZE 8


struct shm_data{
    int count;
    char field[BOARD_SIZE][BOARD_SIZE];
    sem_t players[2];
};

shm_data *g_glb_data = nullptr;


void clean( void )
{
    log_msg( LOG_INFO, "Final cleaning ..." );

    if ( !g_glb_data ) return;

    int l_num_proc = -1;

    /* if ( g_glb_data != nullptr )
    {
        g_glb_data->num_of_process--;
        l_num_proc = g_glb_data->num_of_process;
    } */

    log_msg( LOG_INFO, "Shared memory releasing..." );
    int l_ret = munmap( g_glb_data, sizeof( *g_glb_data ) );
    if ( l_ret )
        log_msg( LOG_ERROR, "Unable to release shared memory!" );
    else
        log_msg( LOG_INFO, "Share memory released." );

    if ( l_num_proc == 0  )
    {
        log_msg( LOG_INFO, "This process is last (pid %d).", getpid() );
        shm_unlink( SHM_NAME );
    }
}


void initialize_board() {
    for(int i = 0; i < BOARD_SIZE; i++) {
        for(int j = 0; j < BOARD_SIZE; j++) {
            g_glb_data->field[i][j] = '.';
        }
    }
}

void show_board(int scl) {
    dprintf(scl, "------------------");
    dprintf(scl, "------------------\n");
    for(int i = 0; i < BOARD_SIZE; i++) {
        for(int j = 0; j < BOARD_SIZE; j++) {
            dprintf(scl,
                "%c", g_glb_data->field[i][j]);
        }
        dprintf(scl, "\n");
    }
}

char buf[128];
char *col;
char *row;

int row_index = -1;
int col_index = -1;

int parse_input(int scl) {
    char *col = strtok(buf, "-");
    char *row = strtok(NULL, "\0");
    row_index = atoi(row);
    char col_inputs[BOARD_SIZE] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H'};

    for(int i = 0; i < BOARD_SIZE; i++ ) {
        if(col_inputs[i] == col[0]){
            col_index = i;
            return 1;
        }
    }
    dprintf(scl, "Zadaj spravny tah\n");
    return 0;
}

void mark(int index) {
    
    g_glb_data->field[row_index][col_index] = index == 0 ? 'X' : 'O';
    
}

void client_handle(int scl) {
    int index = g_glb_data->count;
    g_glb_data->count++;
    dprintf(scl, "Klient c. %d pripojeny\n", g_glb_data->count);

    while(g_glb_data->count < 2){
        dprintf(scl, "Prosim o strpeni\n");
        usleep(2000*1000);

    }

    initialize_board();

    while(1){

        sem_wait(&g_glb_data->players[index]);
        show_board(scl);

        dprintf(scl, "Zadaj tah\n");
        while(1) {
            int n = read(scl, buf, sizeof(buf));

            if(parse_input(scl) ==  1) break;
        }

        mark(index);

        sem_post(&g_glb_data->players[1-index]);
    }
    
}

//***************************************************************************

int main( int t_narg, char **t_args )
{
    if ( t_narg <= 1 ) help( t_narg, t_args );

    int l_port = 0;

    int l_fd = shm_open( SHM_NAME, O_RDWR, 0660 );
    if ( l_fd < 0 )
    {
        log_msg( LOG_ERROR, "Unable to open file for shared memory." );
        l_fd = shm_open( SHM_NAME, O_RDWR | O_CREAT, 0660 );
        if ( l_fd < 0 )
        {
            log_msg( LOG_ERROR, "Unable to create file for shared memory." );
            exit( 1 );
        }
        ftruncate( l_fd, sizeof( shm_data ) );
        log_msg( LOG_INFO, "File created, this process is first" );
        //l_first = 1;
    }

    // share memory allocation
    g_glb_data = ( shm_data * ) mmap( nullptr, sizeof( shm_data ), PROT_READ | PROT_WRITE,
            MAP_SHARED, l_fd, 0 );

    if ( !g_glb_data )
    {
        log_msg( LOG_ERROR, "Unable to attach shared memory!" );
        exit( 1 );
    }
    else
        log_msg( LOG_INFO, "Shared memory attached.");
    
    g_glb_data->count = 0;

    sem_init(&g_glb_data->players[0], 1, 1);
    sem_init(&g_glb_data->players[1], 1, 0);
    
    

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

        // list of fd sources
        pollfd l_read_poll[ 2 ];

        l_read_poll[ 0 ].fd = STDIN_FILENO;
        l_read_poll[ 0 ].events = POLLIN;
        l_read_poll[ 1 ].fd = l_sock_listen;
        l_read_poll[ 1 ].events = POLLIN;

        int client_num = 0;

        while ( 1 ) // wait for new client
        {
            // select from fds
            int l_poll = poll( l_read_poll, 2, -1 );

            if ( l_poll < 0 )
            {
                log_msg( LOG_ERROR, "Function poll failed!" );
                exit( 1 );
            }

            if ( l_read_poll[ 0 ].revents & POLLIN )
            { // data on stdin
                char buf[ 128 ];
                
                int l_len = read( STDIN_FILENO, buf, sizeof( buf) );
                if ( l_len == 0 )
                {
                    log_msg( LOG_DEBUG, "Stdin closed." );
                    exit( 0 );
                }
                if ( l_len < 0 )
                {
                    log_msg( LOG_DEBUG, "Unable to read from stdin!" );
                    exit( 1 );
                }

                log_msg( LOG_DEBUG, "Read %d bytes from stdin", l_len );
                // request to quit?
                if ( !strncmp( buf, STR_QUIT, strlen( STR_QUIT ) ) )
                {
                    log_msg( LOG_INFO, "Request to 'quit' entered.");
                    close( l_sock_listen );
                    exit( 0 );
                }
            }

            if ( l_read_poll[ 1 ].revents & POLLIN )
            { // new client?
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

                if(client_num < 2) {
                    client_num++;
                    log_msg(LOG_INFO, "Pripojeny klient c. %d", client_num);

                    pid_t p = fork();
                if(p == 0) {
                    close(l_sock_listen);
                    client_handle(l_sock_client);
                    close(l_sock_client);
                    exit(0);

                }else if(p > 0){
                    close(l_sock_client);
                }
                }else{
                    dprintf(l_sock_client, "Plno\n");
                    close(l_sock_client);
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

                continue;
            }

        } // while wait for client

        // change source from sock_listen to sock_client
        l_read_poll[ 1 ].fd = l_sock_client;

        while ( 1  )
        { // communication
            char l_buf[ 256 ];

            // select from fds
            int l_poll = poll( l_read_poll, 2, -1 );

            if ( l_poll < 0 )
            {
                log_msg( LOG_ERROR, "Function poll failed!" );
                exit( 1 );
            }

            // data on stdin?
            if ( l_read_poll[ 0 ].revents & POLLIN )
            {
                // read data from stdin
                int l_len = read( STDIN_FILENO, l_buf, sizeof( l_buf ) );
                if ( l_len == 0 )
                {
                    log_msg( LOG_DEBUG, "Stdin closed." );
                    exit( 0 );
                }
                if ( l_len < 0 )
                {
                    log_msg( LOG_ERROR, "Unable to read data from stdin." );
                    exit( 1 );
                }
                else
                    log_msg( LOG_DEBUG, "Read %d bytes from stdin.", l_len );

                // send data to client
                l_len = write( l_sock_client, l_buf, l_len );
                if ( l_len < 0 )
                {
                    log_msg( LOG_ERROR, "Unable to send data to client." );
                    exit( 1 );
                }
                else
                    log_msg( LOG_DEBUG, "Sent %d bytes to client.", l_len );
            }
            // data from client?
            if ( l_read_poll[ 1 ].revents & POLLIN )
            {
                // read data from socket
                int l_len = read( l_sock_client, l_buf, sizeof( l_buf ) );
                if ( l_len == 0 )
                {
                    log_msg( LOG_DEBUG, "Client closed socket!" );
                    close( l_sock_client );
                    break;
                }
                else if ( l_len < 0 )
                {
                    log_msg( LOG_ERROR, "Unable to read data from client." );
                    close( l_sock_client );
                    break;
                }
                else
                    log_msg( LOG_DEBUG, "Read %d bytes from client.", l_len );

                // write data to client
                l_len = write( STDOUT_FILENO, l_buf, l_len );
                if ( l_len < 0 )
                {
                    log_msg( LOG_ERROR, "Unable to write data to stdout." );
                    exit( 1 );
                }

                // close request?
                if ( !strncasecmp( l_buf, "close", strlen( STR_CLOSE ) ) )
                {
                    log_msg( LOG_INFO, "Client sent 'close' request to close connection." );
                    close( l_sock_client );
                    log_msg( LOG_INFO, "Connection closed. Waiting for new client." );
                    break;
                }
            }
            // request for quit
            if ( !strncasecmp( l_buf, "quit", strlen( STR_QUIT ) ) )
            {
                close( l_sock_listen );
                close( l_sock_client );
                log_msg( LOG_INFO, "Request to 'quit' entered" );
                exit( 0 );
                }
        } // while communication
    } // while ( 1 )
    clean();

    return 0;
}
