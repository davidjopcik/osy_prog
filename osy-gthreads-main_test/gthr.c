#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>

#include "gthr.h"

struct gt_context_t g_gttbl[ MaxGThreads ];
struct gt_context_t * g_gtcur;

static uint64_t g_sys_ticks = 0;   

#if ( GT_PREEMPTIVE != 0 )

volatile int g_gt_yield_status = 1;

static void gt_sig_handle( int t_sig );

// initialize and start SIGALRM
static void gt_sig_start( void )
{
    struct sigaction l_sig_act;
    memset( &l_sig_act, 0, sizeof( l_sig_act ) );
    l_sig_act.sa_handler = gt_sig_handle;

    sigaction( SIGALRM, & l_sig_act, NULL );

    struct itimerval l_tv_alarm = { { 0, TimePeriod * 1000 }, { 0, TimePeriod * 1000 } };
    setitimer( ITIMER_REAL, & l_tv_alarm, NULL );
}

// deinitialize and stop SIGALRM
static void gt_sig_stop( void )
{
    struct itimerval l_tv_alarm = { { 0, 0 }, { 0, 0 } };
    setitimer( ITIMER_REAL, & l_tv_alarm, NULL );

    struct sigaction l_sig_act;
    memset( & l_sig_act, 0, sizeof( l_sig_act ) );
    l_sig_act.sa_handler = SIG_DFL;

    sigaction( SIGALRM, & l_sig_act, NULL );
}

static void gt_sig_mask_reset( void )
{
    sigset_t l_set;
    sigemptyset( & l_set );
    sigaddset( & l_set, SIGALRM );
    sigprocmask( SIG_UNBLOCK, & l_set, NULL );
}

static void gt_sig_handle( int t_sig )
{
    (void)t_sig;

    gt_sig_mask_reset();

    gt_manage_timers();

    g_gt_yield_status = gt_yield();
}

#endif


void gt_init( void )
{
    for ( int i = 0; i < MaxGThreads; i++ )
    {
        g_gttbl[i].thread_state = Unused;
        g_gttbl[i].delay_ticks  = 0;
        g_gttbl[i].id           = i;
        g_gttbl[i].name[0]      = '\0';
        g_gttbl[i].cpu_ticks    = 0;
        g_gttbl[i].regs.rsp     = 0;
        g_gttbl[i].regs.rbp     = 0;
#if ( GT_PREEMPTIVE == 0 )
        g_gttbl[i].regs.r15     = 0;
        g_gttbl[i].regs.r14     = 0;
        g_gttbl[i].regs.r13     = 0;
        g_gttbl[i].regs.r12     = 0;
        g_gttbl[i].regs.rbx     = 0;
#endif
    }

    g_sys_ticks = 0;

    //g_gtcur->g_sys_tick = 0;

    g_gtcur = & g_gttbl[ 0 ];
    g_gtcur->thread_state = Running;
    g_gtcur->delay_ticks  = 0;
    g_gtcur->id           = 0;
    snprintf( g_gtcur->name, sizeof( g_gtcur->name ), "Main" );
}

void gt_stop( void )
{
    if ( g_gtcur != & g_gttbl[ 0 ] )
    {
        g_gtcur->thread_state = Unused;
        gt_yield();
        assert( !"reachable" );
    }
}

void gt_start_scheduler( void )
{
#if ( GT_PREEMPTIVE != 0 )
    gt_sig_start();
    while ( g_gt_yield_status )
    {
        usleep( TimePeriod * 1000 + 1000 ); 
    }
    gt_sig_stop();
#else
    while ( gt_yield() ) {}
#endif
}


static int gt_has_ready_nonidle( void )
{
    for ( int i = 1; i < MaxGThreads; i++ )
        if ( g_gttbl[i].thread_state == Ready )
            return 1;
    return 0;
}

int gt_yield( void )
{
    
    if ( g_gtcur && g_gtcur->thread_state == Running )
        g_gtcur->cpu_ticks++;
    g_sys_ticks++;

    struct gt_context_t * p = g_gtcur;
    struct gt_regs * l_old, * l_new;
    int l_no_ready = 0;


/*
    int has_nonidle = gt_has_ready_nonidle();

do
{
    if ( ++p == & g_gttbl[ MaxGThreads ] )
        p = & g_gttbl[ 0 ];

    if ( p == g_gtcur )
        return - l_no_ready;

   
    if ( has_nonidle && p == & g_gttbl[ 0 ] )
        continue;

} while ( p->thread_state != Ready );  */



    int has_nonidle = gt_has_ready_nonidle();

    while ( p->thread_state != Ready )
    {
        if ( p->thread_state == Blocked || p->thread_state == Suspended )
            l_no_ready++;

        if ( ++p == & g_gttbl[ MaxGThreads ] )
            p = & g_gttbl[ 0 ];

        if ( p == g_gtcur )
            return - l_no_ready;

        if ( has_nonidle && p == & g_gttbl[ 0 ] )
        {
            
        }
    }

    if ( has_nonidle && p == & g_gttbl[ 0 ] )
    {
        for ( int i = 1; i < MaxGThreads; i++ )
        {
            if ( g_gttbl[i].thread_state == Ready )
            {
                p = & g_gttbl[i];
                break;
            }
        }
    }

    if ( g_gtcur->thread_state == Running )
        g_gtcur->thread_state = Ready;

    p->thread_state = Running;

    l_old = & g_gtcur->regs;
    l_new = & p->regs;
    g_gtcur = p;
    g_sys_ticks++;

#if ( GT_PREEMPTIVE != 0 )
    gt_pree_swtch( l_old, l_new );
#else
    gt_swtch( l_old, l_new );
#endif

    return 1;
}


int gt_go( const char *t_name, void ( * t_run )( void ) )
{
    char * l_stack;
    struct gt_context_t * p;

    for ( p = & g_gttbl[ 0 ];; p++ )
    {
        if ( p == & g_gttbl[ MaxGThreads ] )
            return -1;
        else if ( p->thread_state == Unused )
            break;
    }

    int idx = (int)( p - g_gttbl );

    l_stack = (char*) malloc( StackSize );
    if ( !l_stack )
        return -1;

    *(uint64_t*)&l_stack[ StackSize - 8  ] = (uint64_t) gt_stop;
    *(uint64_t*)&l_stack[ StackSize - 16 ] = (uint64_t) t_run;

    p->regs.rsp      = (uint64_t) & l_stack[ StackSize - 16 ];
    p->thread_state  = Ready;
    p->delay_ticks   = 0;
    p->id            = idx;
    p->cpu_ticks     = 0;

    if ( t_name )
    {
        strncpy( p->name, t_name, sizeof( p->name ) - 1 );
        p->name[ sizeof( p->name ) - 1 ] = '\0';
    }
    else
    {
        snprintf( p->name, sizeof( p->name ), "T%d", idx );
    }

    return idx;
}


int uninterruptibleNanoSleep( time_t t_sec, long t_nanosec )
{
    struct timespec req;
    req.tv_sec  = t_sec;
    req.tv_nsec = t_nanosec;

    do {
        if ( 0 != nanosleep( & req, & req ) )
        {
            if ( errno != EINTR )
                return -1;
        }
        else
        {
            break;
        }
    } while ( req.tv_sec > 0 || req.tv_nsec > 0 );

    return 0;
}


void gt_suspend( void )
{
    g_gtcur->thread_state = Suspended;
    gt_yield();
}

void gt_delay( int t_ms )
{
    if ( t_ms <= 0 )
        return;

    int ticks = t_ms / TimePeriod;
    if ( ticks <= 0 )
        ticks = 1;

    g_gtcur->delay_ticks  = ticks;
    g_gtcur->thread_state = Blocked;

    gt_yield();
}

void gt_resume( int t_id )
{
    if ( t_id < 0 || t_id >= MaxGThreads )
        return;

    struct gt_context_t * p = & g_gttbl[ t_id ];

    if ( p->thread_state == Suspended )
        p->thread_state = Ready;
}

void gt_manage_timers( void )
{
    for ( int i = 0; i < MaxGThreads; i++ )
    {
        struct gt_context_t * p = & g_gttbl[i];

        if ( p->thread_state == Blocked && p->delay_ticks > 0 )
        {
            p->delay_ticks--;

            if ( p->delay_ticks <= 0 )
            {
                p->delay_ticks = 0;
                p->thread_state = Ready;
            }
        }
    }
}

static const char * gt_state_str( gt_thread_state_t s )
{
    switch ( s )
    {
        case Unused:    return "Unused";
        case Running:   return "Running";
        case Ready:     return "Ready";
        case Blocked:   return "Blocked";
        case Suspended: return "Suspended";
        default:        return "Unknown";
    }
}

void gt_task_list( void )
{
    printf( "ID  State      Ticks     Time(ms)   Name\n" );
    printf( "----------------------------------------------\n" );

    for ( int i = 0; i < MaxGThreads; i++ )
    {
        struct gt_context_t * p = & g_gttbl[i];

        if ( p->thread_state != Unused )
        {
            char mark = ( p == g_gtcur ) ? '*' : ' ';
            uint64_t ms = p->cpu_ticks * (uint64_t)TimePeriod;
            printf( "%c%2d %-9s %8llu %10llu   %s\n",
                    mark,
                    p->id,
                    gt_state_str( p->thread_state ),
                    //(unsigned long long)p->cpu_ticks,
                    (unsigned long long)g_sys_ticks,
                    (unsigned long long)ms,
                    p->name );
        }
    }

    printf( "\nSYSTEM: ticks=%llu  cpu_ticks=%llu\n\n  time(ms)=%llu\n\n"  ,
            (unsigned long long)g_sys_ticks,
            (unsigned long long)g_gtcur->cpu_ticks,
            (unsigned long long)( g_sys_ticks * (uint64_t)TimePeriod ) );
}



