#ifndef __GTHR_H
#define __GTHR_H

#include <stdint.h>
#include <time.h>

#ifndef GT_PREEMPTIVE
#define GT_PREEMPTIVE 1
#endif

enum {
    MaxGThreads = 12,               
    StackSize   = 0x400000,
    TimePeriod  = 10,               
};

typedef enum {
    Unused,
    Running,
    Ready,
    Blocked,
    Suspended,
} gt_thread_state_t;

struct gt_context_t {
    struct gt_regs {
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

    gt_thread_state_t thread_state;

    int id;
    int delay_ticks;
    char name[32];

    uint64_t cpu_ticks;     
};

extern struct gt_context_t g_gttbl[ MaxGThreads ];
extern struct gt_context_t * g_gtcur;

void gt_init( void );
int  gt_go( const char *t_name, void ( * t_run )( void ) );
void gt_stop( void );
int  gt_yield( void );
void gt_start_scheduler( void );

void gt_swtch( struct gt_regs * t_old, struct gt_regs * t_new );
void gt_pree_swtch( struct gt_regs * t_old, struct gt_regs * t_new );

int  uninterruptibleNanoSleep( time_t sec, long nanosec );

void gt_delay( int t_ms );
void gt_suspend( void );
void gt_resume( int t_id );
void gt_task_list( void );

void gt_manage_timers( void );

#endif // __GTHR_H
