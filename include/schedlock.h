#ifndef _FIBER_SCHED_LOCK_H_
#define _FIBER_SCHED_LOCK_H_

#include <stdio.h>                  
#include <stdbool.h>        
#include <stdatomic.h>                   
#include "timing.h"         
#include "fiber_manager.h"  
#include "fairlock.h" 
#include "fiber_mutex.h"  
#include "fiber_spinlock.h"    

// static struct timeval inactive_threshold = {1, 0}; 

struct sched_lock {
    struct timeval start_ticks;
    struct timeval end_ticks;
    struct timeval slice_end_time;
    // fiber_mutex_t mutex;
    fiber_spinlock_t spinlock;

} sched_lock_t; 

void sched_lock_init(struct sched_lock *lock)
{
    //fiber_mutex_init(&lock->mutex);
    fiber_spinlock_init(&lock->spinlock);
    lock->start_ticks = (struct timeval){0, 0};
    lock->end_ticks = (struct timeval){0, 0};
    lock->slice_end_time = (struct timeval){0, 0};

}

void sched_lock_acquire(struct sched_lock *lock)
{
    // Colour if UnColoured and Record Lock.
    set_fiber_colour(0, (void*)lock);
    
    // Lock the underlying mutex.
    // fiber_mutex_lock(&lock->mutex);
    fiber_spinlock_lock(&lock->spinlock);

    // Record the start time.
    gettimeofday(&lock->start_ticks, NULL);
    
    // Retrieve the fiber's lock statistics.
    lock_stats_t* fiber_stats = get_lock_fiber_data((void*)lock);
    
    // If no stats exist, insert one with a default slice value.
    if (!fiber_stats) {
        printf("Error: No Stats, something is wrong.\n");
        abort();
    }

    // Compute the slice end time.
    timeval_add(&lock->slice_end_time, &lock->start_ticks, &fiber_stats->slice_size);
}

void sched_lock_release(struct sched_lock *lock)
{
    // We don't unset the colour and assume the thread can acquire this lock again 
    // Lock Release Mechanism 
    struct timeval now;
    struct timeval time_adder;
    struct timeval banned_until;
    unsigned long long cs_length;
    fiber_spinlock_unlock(&lock->spinlock); // find a way for it to enter if a fiber holds a lock
    // fiber_mutex_unlock(&lock->mutex);
    gettimeofday(&now, NULL);
    if (timercmp(&now, &lock->slice_end_time,>)){ // enter if slice has expired
        int nthreads = get_fiber_count();
        // fiber_mutex_unlock(&lock->mutex);
        gettimeofday(&lock->end_ticks, NULL);
        if (nthreads > 1) {
            /* Expand ban tvime by (cs_length * num_threads). */
            cs_length = time_diff(lock->start_ticks, lock->end_ticks);
            time_adder.tv_sec  = (cs_length * nthreads) / 1000000ULL;
            time_adder.tv_usec = (cs_length * nthreads) % 1000000ULL;
    
            timeval_add(&banned_until, &lock->end_ticks, &time_adder);
            set_lock_fiber_data((void*)lock, &banned_until, NULL);
            }
         else {
            /* If only one fiber, no ban needed. */
            set_lock_fiber_data((void*)lock, &lock->end_ticks, NULL);
        }
        fiber_yield(); // Yield to Allow Others to get resources 
        return;
    }
    return;
}


#endif