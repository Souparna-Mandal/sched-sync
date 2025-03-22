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

#define SLICE_TIME_US 200

// static struct timeval inactive_threshold = {1, 0}; 

struct sched_lock {
    struct timeval start_ticks;
    struct timeval end_ticks;
    struct timeval slice_end_time;
    struct timeval slice_size;
    // fiber_mutex_t mutex;
    fiber_spinlock_t spinlock;
    int slice_time_set; 

} sched_lock_t; 

void sched_lock_init(struct sched_lock *lock)
{
    //fiber_mutex_init(&lock->mutex);
    fiber_spinlock_init(&lock->spinlock);
    lock->start_ticks = (struct timeval){0, 0};
    lock->end_ticks = (struct timeval){0, 0};
    lock->slice_end_time = (struct timeval){0, 0};
    lock->slice_size = (struct timeval){0, SLICE_TIME_US};
    lock->slice_time_set = 0;
}

void sched_lock_acquire(struct sched_lock *lock)
{
    // Colour if UnColoured and Record Lock.
    set_fiber_colour((void*)lock); 

    // fiber_spinlock_lock(&lock->spinlock);
    // Retrieve the fiber's lock statistics.
    // If no stats exist, insert one with a default slice value.
    // if (!fiber_stats) {
    //     printf("Error: No Stats, something is wrong.\n");
    //     abort();
    // }
    if (lock->slice_time_set == 0){
        gettimeofday(&lock->start_ticks, NULL);
        timeval_add(&lock->slice_end_time, &lock->start_ticks, &lock->slice_size); // slice expiry time
        lock->slice_time_set = 1;
    }
}

void sched_lock_release(struct sched_lock *lock)
{
    // We don't unset the colour and assume the thread can acquire this lock again 
    // Lock Release Mechanism 
    struct timeval now;
    unsigned long long banned_until;
    unsigned long long cs_length;
    // fiber_spinlock_unlock(&lock->spinlock); // find a way for it to enter if a fiber holds a lock
    
    gettimeofday(&now, NULL); // ban time is always frikking the same
    // printf("Now: %ld.%06ld, Slice End: %ld.%06ld\n",
    //     now.tv_sec, now.tv_usec, lock->slice_end_time.tv_sec, lock->slice_end_time.tv_usec);
    if (timercmp(&now, &lock->slice_end_time, >))
    { // enter if slice has expired
        lock->slice_time_set = 1;
        int nthreads = get_fiber_count();
        gettimeofday(&lock->end_ticks, NULL);
        if (nthreads > 1) {
            /* Expand ban tvime by (cs_length * num_threads). */
            cs_length = time_diff(lock->start_ticks, lock->end_ticks);
            banned_until = cs_length * nthreads;
        }
         else {
            /* If only one fiber, no ban needed. */
            banned_until = timeval_to_ull(lock->end_ticks);
        }
        set_lock_fiber_data((void*)lock, banned_until);
        // Unset Colour Here
        unset_colour((void*)lock );
        fiber_yield(); // Yield to Allow Others to acquire the lock
        return;
    }
    return;
}


#endif