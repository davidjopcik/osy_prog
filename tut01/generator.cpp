#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main( int t_argn, char **t_argc )
{
    //for( int i = 0; i < t_argn; i++ )
    //    printf( "%s\n", t_argc[ i ] );

    if ( t_argn < 2 )
    {
        printf( "Malo parametru!\n" );
        exit( 1 );
    }

    int n = atoi( t_argc[ 1 ] );

    for ( int i = 0; i < n; i++ )
        printf( "%d\n", rand() % 100000 );
}
