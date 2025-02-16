#ifndef __SCHEDLOCK_H__
#define __SCHEDLOCK_H__

#include <stdio.h>                  
#include <stdbool.h>        
#include <stdatomic.h>                     
#include "timing.h"         
#include "fiber_manager.h"  
#include "fairlock.h"       

typedef unsigned long long ull;
static struct timeval sleep_granuality_tv = {0,SLEEP_GRANULARITY};

#define spin_then_yield(limit, expr) while (1) { \
    int val, counter = 0;                        \
    while ((val = (expr)) && counter++ < limit); \
    if (!val) break; fiber_yield(); }


enum node_state {
    INIT = 0, // not waiting or after next runnable node
    NEXT,
    RUNNABLE,
    RUNNING
};

typedef struct waiter{
    ull id;
    struct timeval slice;
    struct timeval start_ticks;
    struct timeval end_ticks;
} waiter_t; 

typedef struct qnode { // linked list waiter 
    int state __attribute__ ((aligned (CACHELINE)));
    waiter_t *waiter;
    struct qnode *next __attribute__ ((aligned (CACHELINE)));
} qnode_t __attribute__ ((aligned (CACHELINE)));

typedef struct fairlock {
    qnode_t *qtail __attribute__ ((aligned (CACHELINE)));
    qnode_t *qnext __attribute__ ((aligned (CACHELINE)));
    waiter_t *cur_waiter __attribute__ ((aligned (CACHELINE))); // current waiter object
    struct timeval slice __attribute__((aligned(CACHELINE)));
} fairlock_t __attribute__ ((aligned (CACHELINE)));

static inline qnode_t *flqnode(fairlock_t *lock) {
    return (qnode_t *) ((char *) &lock->qnext - offsetof(qnode_t, next));
}

int fairlock_init(fairlock_t *lock) {
    lock->qtail = NULL;
    lock->qnext = NULL;
    lock->slice = {0,0};
    lock->cur_waiter = NULL;
    return 0;
}

static waiter_t *waiter_create(){
    waiter_t *waiter;
    waiter = malloc(sizeof(waiter_t));
    waiter->slice = {0,0};
    waiter->start_ticks = {0,0};
    return waiter;
}

int fairlock_destroy(fairlock_t *lock) {
    //return pthread_key_delete(lock->flthread_info_key);
    return 0;
}

void fairlock_acquire(fairlock_t *lock) {
    waiter_t *waiter;
    struct timeval now;
    waiter = lock->cur_waiter;
    if (NULL == waiter) {
        waiter = waiter_create();
    }

    if (isslicevalid(fairlock_t* lock)) { // given a lock and fiber, is the slice valid ? NEED TO IMPLEMENT

        struct timeval curr_slice = lock->slice; // get the current slice owner
        // If owner of current slice, try to reenter at the beginning of the queue
        gettimeofday(&now, NULL);
        if ((curr_slice == waiter->slice) && (timercmp(&now, &info->slice, <))) {
            qnode_t *succ = readvol(lock->qnext);
            if (NULL == succ) {
                if (atomic_compare_exchange_strong(&lock->qtail, NULL, flqnode(lock)))
                    goto reenter;
                spin_then_yield(SPIN_LIMIT, timercmp(&now, &curr_slice,<) && NULL == (succ = readvol(lock->qnext)));
                // let the succ invalidate the slice, and don't need to wake it up because slice expires naturally
                if (now >= curr_slice)
                    goto begin;
            }
            // if state < RUNNABLE, it won't become RUNNABLE unless someone releases lock,
            // but as no one is holding the lock, there is no race
            if (succ->state < RUNNABLE || atomic_compare_exchange_strong(&succ->state, RUNNABLE, NEXT)) {
                info->start_ticks = now;
                return;
            }
        }
    }
begin:

    if (isbanned(fairlock_t* lock)) { // given a lock and a fiber, is the fiber banned? NEED TO IMPLEMENT
        struct timeval* banned_until = get_ban_time(fairlock_t* lock); // NEED TO IMPLEMENT
        gettimeofday(&now, NULL);
        if (timercmp(&now, banned_until, <)) {
            ull banned_time = time_diff(now, *banned_until);

            // sleep with granularity of SLEEP_GRANULARITY us
            while (timercmp(&banned_time, &sleep_granuality_tv, >)) {
                struct timespec req = {
                    .tv_sec = banned_time / 1000000,
                    .tv_nsec = (banned_time % 1000000 / SLEEP_GRANULARITY) * SLEEP_GRANULARITY,
                };
                nanosleep(&req, NULL);
                gettimeofday(&now, NULL);
                if (timercmp(&banned_until, &now, <=))
                    break;
                banned_time = time_diff(now, *banned_until);
            }
            // spin for the remaining (<SLEEP_GRANULARITY us)
            spin_then_yield(SPIN_LIMIT, timercmp(&now, &banned_until,<));
        }
    }
    
    qnode_t n = { 0, waiter};
    while (1) {
        qnode_t *prev = readvol(lock->qtail);
        if (atomic_compare_exchange_strong(&lock->qtail, prev, &n)) {
            // enter the lock queue
            if (NULL == prev) {
                n.state = RUNNABLE;
                lock->qnext = &n;
            } else {
                if (prev == flqnode(lock)) {
                    n.state = NEXT;
                    prev->next = &n;
                } else {
                    prev->next = &n;
                    while (INIT == readvol(n.state)){} // replaced futex, make efficient later
                }
            }

            // wait until the current slice expires
            int slice_valid = isslicevalid(fairlock_t* lock);
            struct timeval curr_slice = readvol (lock->slice);
            gettimeofday(&now, NULL);
            struct timeval* checker = time_diff(&now,sleep_granuality_tv)
            
            while (slice_valid  && timercmp(&checker,&curr_slice),>) {
                ull slice_left = curr_slice - now;
                struct timespec timeout = {
                    .tv_sec = 0, // slice will be less then 1 sec
                    .tv_nsec = (slice_left / (CYCLE_PER_US * SLEEP_GRANULARITY)) * SLEEP_GRANULARITY * 1000,
                };
                // futex(&lock->slice_valid, FUTEX_WAIT_PRIVATE, 0, &timeout);

                /*Loop Update*/
                slice_valid = isslicevalid(fairlock_t* lock);
                gettimeofday(&now, NULL);
                curr_slice = readvol (lock->slice);
                struct timeval* checker = time_diff(&now,sleep_granuality_tv)
            }
            if (slice_valid) {
                spin_then_yield(SPIN_LIMIT, (slice_valid = isslicevalid(fairlock_t* lock) && rdtsc() < readvol(lock->slice)));
                if (slice_valid)
                    lock->slice_valid = 0;
            }
            // invariant: rdtsc() >= curr_slice && lock->slice_valid == 0
spin_then_yield
#ifdef DEBUG
            now = rdtsc();
#endif
            // spin until RUNNABLE and try to grab the lock
            spin_then_yield(SPIN_LIMIT, RUNNABLE != readvol(n.state) || 0 == __sync_bool_compare_and_swap(&n.state, RUNNABLE, RUNNING));
            // invariant: n.state == RUNNING
#ifdef DEBUG
            info->stat.runnable_wait += rdtsc() - now;
#endif

            // record the successor in the lock so we can notify it when we release
            qnode_t *succ = readvol(n.next);
            if (NULL == succ) {
                lock->qnext = NULL;
                if (0 == __sync_bool_compare_and_swap(&lock->qtail, &n, flqnode(lock))) {
                    spin_then_yield(SPIN_LIMIT, NULL == (succ = readvol(n.next)));
#ifdef DEBUG
                    info->stat.succ_wait += rdtsc() - now;
#endif
                    lock->qnext = succ;
                }
            } else {
                lock->qnext = succ;
            }
            // invariant: NULL == succ <=> lock->qtail == flqnode(lock)

            now = rdtsc();
            info->start_ticks = now;
            info->slice = now + FAIRLOCK_GRANULARITY;
            lock->slice = info->slice;
            lock->slice_valid = 1;
            // wake up successor if necessary
            if (succ) {
                succ->state = NEXT;
                futex(&succ->state, FUTEX_WAKE_PRIVATE, 1, NULL);
            }
            return;
        }
    }
}

void fairlock_release(fairlock_t *lock) {
    ull now, cs;
#ifdef DEBUG
    ull succ_start = 0, succ_end = 0;
#endif
    flthread_info_t *info;

    qnode_t *succ = lock->qnext;
    if (NULL == succ) {
        if (__sync_bool_compare_and_swap(&lock->qtail, flqnode(lock), NULL))
            goto accounting;
#ifdef DEBUG
        succ_start = rdtsc();
#endif
        spin_then_yield(SPIN_LIMIT, NULL == (succ = readvol(lock->qnext)));
#ifdef DEBUG
        succ_end = rdtsc();
#endif
    }
    succ->state = RUNNABLE;

accounting:
    // invariant: NULL == succ || succ->state = RUNNABLE
    info = (flthread_info_t *) pthread_getspecific(lock->flthread_info_key);
    now = rdtsc();
    cs = now - info->start_ticks;
    info->banned_until += cs * (__atomic_load_n(&lock->total_weight, __ATOMIC_RELAXED) / info->weight);
    info->banned = now < info->banned_until;

    if (info->banned) {
        if (__sync_bool_compare_and_swap(&lock->slice_valid, 1, 0)) {
            futex(&lock->slice_valid, FUTEX_WAKE_PRIVATE, 1, NULL);
        }
    }
#ifdef DEBUG
    info->stat.release_succ_wait += succ_end - succ_start;
#endif
}

#endif // __FAIRLOCK_H__