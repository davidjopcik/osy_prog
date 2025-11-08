#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

int main()
{
    int roura1[ 2 ];
    pipe( roura1 );

    if ( fork() == 0 )
    {
        // potomek
        dup2( roura1[ 1 ], 1 );
        close( roura1[ 0 ] );
        close( roura1[ 1 ] );
        execlp( "ls", "ls", "/home/fei/oli10/poli/", "/var/", NULL );
        printf( "pokud jsem zde, neco je spatne\n" );
        exit( 1 );

    }

    if ( fork() == 0 )
    {
        int fd = open( "out.txt", O_RDWR | O_CREAT | O_TRUNC, 0644 );
        dup2( fd, 1 );
        close( fd );
        dup2( roura1[ 0 ], 0 );
        close( roura1[ 0 ] );
        close( roura1[ 1 ] );
        execlp( "tr", "tr", "a-z", "A-Z", NULL );
        printf( "pokud jsem zde, neco je spatne\n" );
        exit( 1 );

    }

    close( roura1[ 0 ] );
    close( roura1[ 1 ] );

    wait( NULL );
    wait( NULL );

    //while ( 1 )
    //{
    //    char buf[ 135 ];
    //    int len = read( roura1[ 0 ], buf, sizeof( buf ) );
    //    if ( len <= 0 ) break;
    //    write( 1, buf, len );
    //}
}
