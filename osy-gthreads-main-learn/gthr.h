#ifndef __GTHR_H
#define __GTHR_H

#include <stdint.h>
#include <time.h>

#ifndef GT_PREEMPTIVE
#define GT_PREEMPTIVE 1
#endif

enum {
    MaxGThreads = 5,                // Maximum number of threads
    StackSize   = 0x400000,         // Size of stack of each thread
    TimePeriod  = 10,               // Timer period in ms
};

// Thread state
typedef enum {
    Unused,
    Running,
    Ready,
    Blocked,
    Suspended,
} gt_thread_state_t;

// Thread context
struct gt_context_t {
    struct gt_regs {                // Saved context
        uint64_t rsp;
        uint64_t rbp;
#if ( GT_PREEMPTIVE == 0 )
        uint64_t r15;
        uint64_t r14;
        uint64_t r13;
        uint64_t r12;
        uint64_t rbx;
#endif
    } regs;

    gt_thread_state_t thread_state; // state
    int id;                         // id = index v tabuľke
    int delay_ticks;                // ostávajúce tick-y pri delay
    char name[32];                  // meno vlákna
};

// API
void gt_init( void );                                // init gttbl
void gt_stop( void );                                // terminate current thread
int  gt_yield( void );                               // yield a prepnúť
void gt_start_scheduler( void );                     // spustiť „jadro“

void gt_swtch( struct gt_regs * t_old,
               struct gt_regs * t_new );
void gt_pree_swtch( struct gt_regs * t_old,
                    struct gt_regs * t_new );

int  uninterruptibleNanoSleep( time_t sec,
                               long nanosec );

// nové API podľa zadania
int  gt_go( const char * t_name,
            void ( * t_run )( void ) );              // create thread

void gt_delay( int t_ms );                           // Blocked na čas
void gt_suspend( void );                             // Suspended
void gt_resume( int t_id );                          // Suspended -> Ready
void gt_task_list( void );                           // výpis

#endif // __GTHR_H