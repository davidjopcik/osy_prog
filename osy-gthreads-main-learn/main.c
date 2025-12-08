#include <stdio.h>
#include "gthr.h"

// id vlákna Sleep, aby ho Ring vedel zobudiť
int sleep_id;

void task_hello( void )
{
    while ( 1 )
    {
        printf( "Hello world!\n" );
        gt_delay( 1000 );   // cca 1000 ms (1 sekunda)
    }
}

void task_sleep( void )
{
    while ( 1 )
    {
        gt_suspend();       // Suspended, čaká na gt_resume(sleep_id)
        printf( "Please do not wake up!\n" );
    }
}

void task_ring( void )
{
    while ( 1 )
    {
        gt_delay( 5000 );   // cca 5 sekúnd
        printf( "Ring! Ring! Ring!\n" );
        gt_resume( sleep_id );
    }
}

int main( void )
{
    gt_init();

    gt_go( "Hello", task_hello );
    sleep_id = gt_go( "Sleep", task_sleep );
    gt_go( "Ring",  task_ring );

    // voliteľne: hneď na začiatku si môžeš vypísať zoznam
    gt_task_list();

    gt_start_scheduler();

    printf( "Threads finished\n" );
    return 0;
}