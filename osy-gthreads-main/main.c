//***************************************************************************
//
// GThreads - preemptive threads in userspace. 
// Inspired by https://c9x.me/articles/gthreads/code0.html.
//
// Program created for subject OSMZ and OSY. 
//
// Michal Krumnikl, Dept. of Computer Sciente, michal.krumnikl@vsb.cz 2019
// Petr Olivka, Dept. of Computer Science, petr.olivka@vsb.cz, 2021
//
// Program simulates preemptice switching of user space threads. 
//
//***************************************************************************

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include "gthr.h"

int sleep_id;

void task_hello( void ) {
    while(1) {
        //gt_task_list();
        printf( "Hello world!\n" );
        gt_delay( 1000 );
    }
}

void task_sleep( void ) {
    while( 1 ) {
        gt_suspend();
        printf( "Please do not wake up!\n" );
        //gt_task_list();
    }
}

void task_ring( void ) {
    while(1) {
        gt_delay( 5000 );
        printf( "Ring! Ring! Ring!\n" );
        //gt_task_list();
        gt_resume( sleep_id );
    }
}



int main(void) {
    gt_init();      // initialize threads, see gthr.c

    gt_go( "Hello", task_hello);
    sleep_id = gt_go("Sleep", task_sleep);
    gt_go("Ring", task_ring);
    gt_task_list();

    gt_start_scheduler(); // wait until all threads terminate
  
    printf( "Threads finished\n" );
}
