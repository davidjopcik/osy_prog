#include <stdio.h>
#include <stdlib.h>
#include "gthr.h"

#define COUNT 1000

static int sleep_id = -1;

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
    gt_task_list();
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





void task_hello( void )
{
    while ( 1 )
    {
        printf( "Hello world!\n" );
        
       
        static int k = 0;
        k++;
        if (k % 5 == 0) gt_task_list();   

                gt_delay( 1000 );
            }
}

void task_sleep( void )
{
    while ( 1 )
    {
        gt_suspend();
        printf( "Please do not wake up!\n" );
    }
}

void task_ring( void )
{
    while ( 1 )
    {
        gt_delay( 5000 );
        printf( "Ring! Ring! Ring!\n" );
        gt_resume( sleep_id );
    }
}

void busy( void )
{
    while (1)
    {
        
    }
}


int main( void )
{
    /*  
    gt_init();
    gt_go("Busy", busy);
    gt_task_list();
    gt_start_scheduler(); */

    
    gt_init();



    gt_go( "f", f );
    gt_go( "f", f );
    gt_go( "g", g );
    gt_go( "g", g );

    gt_go( "Hello", task_hello );
    sleep_id = gt_go( "Sleep", task_sleep );
    gt_go( "Ring", task_ring );

    gt_task_list();

    gt_start_scheduler();

    return 0;
}
