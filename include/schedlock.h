#include <stdio.h>                  
#include <stdbool.h>        
#include <stdatomic.h>                   
#include "timing.h"         
#include "fiber_manager.h"  
#include "fairlock.h"       

static struct timeval inactive_threshold = {1, 0}; 

struct sched_lock {
    struct timeval start_ticks;
    struct timeval end_ticks;
    struct timeval slice_end_time;
} sched_lock_t; 

void sched_lock_init(struct sched_lock *lock)
{
    lock->start_ticks = (struct timeval){0, 0};
    lock->end_ticks = (struct timeval){0, 0};
    lock->slice_end_time = (struct timeval){0, 0};
}

void sched_lock_acquire(struct sched_lock *lock)
{
    gettimeofday(&lock->start_ticks, NULL);
    // peep in and get slice_time
    lock_stats_t* fiber_stats = get_current_fiber_stats();
    timeval_add(&lock->slice_end_time, &lock->start_ticks, &fiber_stats->banned_until);
}

void sched_lock_release(struct sched_lock *lock, int nthreads)
{
    struct timeval now;
    struct timeval time_adder;
    struct timeval banned_until;
    unsigned long long cs_length;
    gettimeofday(&now, NULL);
    if (timercmp(&now, &lock->slice_end_time,>)){ // enter if slice has expired
        gettimeofday(&lock->end_ticks, NULL);
        if (nthreads > 1) {
            /* Expand ban time by (cs_length * num_threads). */
            cs_length = time_diff(lock->start_ticks, lock->end_ticks);
            time_adder.tv_sec  = (cs_length * nthreads) / 1000000ULL;
            time_adder.tv_usec = (cs_length * nthreads) % 1000000ULL;
    
            timeval_add(&banned_until, &lock->end_ticks, &time_adder);
            set_current_lock_stats(&banned_until, NULL);
            }
         else {
            /* If only one fiber, no ban needed. */
            set_current_lock_stats(&lock->end_ticks, NULL);
        }
        fiber_yield();
        return;
    }
    return;
}
