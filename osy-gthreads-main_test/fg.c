#include <stdio.h>
#include "gthr.h"

#define COUNT 100

void f( void )
{
    static int x;
    int count = COUNT;
    int id = ++x;

    while ( count-- )
    {
        printf( "F Thread id: %d count: %d\n", id, count );
        uninterruptibleNanoSleep( 0, 1000000 );
#if ( GT_PREEMPTIVE == 0 )
        gt_yield();
#endif
    }
}

void g( void )
{
    static int x;
    int count = COUNT;
    int id = ++x;

    while ( count-- )
    {
        printf( "G Thread id: %d count: %d\n", id, count );
        uninterruptibleNanoSleep( 0, 1000000 );
#if ( GT_PREEMPTIVE == 0 )
        gt_yield();
#endif
    }
}

int main( void )
{
    gt_init();

    gt_go( "f", f );
    gt_go( "g", g );

    gt_task_list();

    gt_start_scheduler();

    return 0;
}
