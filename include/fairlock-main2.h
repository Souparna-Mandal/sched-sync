#include <stdio.h>                  
#include <stdbool.h>        
#include <stdatomic.h>           
#include "hashmap.h"       
#include "list.h"           
#include "timing.h"         
#include "fiber_manager.h"  
#include "fairlock.h"       

static struct timeval inactive_threshold = {1, 0}; // 1 second

struct fairlock_waiter {
    struct timeval banned_until;
    struct timeval start_ticks;
    struct timeval end_ticks;
    struct hlist_node hash;   
    struct list_head list;    
    int fid;             // Fiber ID
};


static inline struct fairlock_waiter *create_waiter(struct fairlock *lock, int fid_c)
{
    struct fairlock_waiter *waiter;
    struct timeval now;

    gettimeofday(&now, NULL);
    waiter = (struct fairlock_waiter *)malloc(sizeof(*waiter));
    if (!waiter) {
        fprintf(stderr, "Error: Unable to allocate fairlock_waiter\n");
        return NULL;
    }

    waiter->fid          = fid_c;
    waiter->banned_until = now;
    waiter->start_ticks  = now;
    waiter->end_ticks    = now;
    INIT_LIST_HEAD(&waiter->list);
    list_add_tail(&waiter->list, &lock->waiters); // adding the waiters node from lock 
    INIT_HLIST_NODE(&waiter->hash);
    hash_add(lock->waiters_lookup, &waiter->hash, fid_c);
    atomic_fetch_add(&lock->num_threads, 1);
    return waiter;
}

static inline struct fairlock_waiter *retrieve_waiter(struct fairlock *lock, int fid)
{
    struct fairlock_waiter *waiter;

    /* hash_for_each_possible => checks only the bucket for the given key */
    hash_for_each_possible(lock->waiters_lookup, waiter, hash, fid) {
        if (waiter->fid == fid) {
            return waiter;
        }
    }
    return NULL;
}

void fairlock_init(struct fairlock *lock)
{
    hash_init(lock->waiters_lookup);
    INIT_LIST_HEAD(&lock->waiters);

    atomic_init(&lock->num_threads, 0);
    atomic_init(&lock->next_ticket, 0);
    atomic_init(&lock->now_serving, 0);

    lock->holder = NULL;
}

void fairlock_destroy(struct fairlock *lock)
{
    unsigned int end_ticket;

    /*
     * Grab the next_ticket value. This increments next_ticket by 1
     * and returns the old value. We store it in end_ticket.
     */
    end_ticket = atomic_fetch_add(&lock->next_ticket, 1);

    /* Wait until now_serving catches up (i.e., all waiters done). */
    while (atomic_load(&lock->now_serving) != end_ticket) {
        fiber_yield();
    }
}

int fair_trylock(struct fairlock *lock, int fid)
{
    unsigned int my_ticket;
        unsigned int serving;
    struct fairlock_waiter *waiter;

    /* Load current 'now_serving' and try to do a compare_exchange on next_ticket. */
    serving = atomic_load(&lock->now_serving);

    if (serving != atomic_compare_exchange_strong(&lock->next_ticket, &serving, serving + 1))
		return 0;

	my_ticket = serving;

    waiter = retrieve_waiter(lock, fid);
    
    if (!waiter) {
        /* No existing waiter => create one */
        waiter = create_waiter(lock, fid);
        if (!waiter) {
            return 0;
        }
    } else {
        /* The waiter already exists => check if it's still banned. */
        struct timeval cur_time;
        gettimeofday(&cur_time, NULL);

        if (timercmp(&waiter->end_ticks, &waiter->banned_until, <) &&
            timercmp(&cur_time, &waiter->banned_until, <)) {
            /* The waiter is banned until some future time => yield lock. */
            atomic_fetch_add(&lock->now_serving, 1);
            return 0;
        }
        /* Otherwise, we can start a new critical section. */
        gettimeofday(&waiter->start_ticks, NULL);
    }

    lock->holder = waiter;
    return 1; /* Successfully acquired */
}

/* --------------------------------------------------------------------------
 * fair_lock
 *
 * Blocking version: we spin (yield) until it's our ticket, 
 * then re-check ban if needed.
 * -------------------------------------------------------------------------- */
void fair_lock(struct fairlock *lock, int fid)
{
    unsigned int my_ticket;
    struct fairlock_waiter *waiter;

    /*Become the next waiting thread to get the lock */
    my_ticket = atomic_fetch_add(&lock->next_ticket, 1);

    /* Spin-wait until its our turn to get lock */
    while (atomic_load(&lock->now_serving) != my_ticket) {
    }

    /* Now we hold the lock from a ticket perspective. */
    waiter = retrieve_waiter(lock, fid);

    if (!waiter) {
        /*Create a new Waiter struct, and give this fiber the lock*/
        waiter = create_waiter(lock, fid);
        if (!waiter) {
            fprintf(stderr, "Unable to allocate memory for fairlock waiter\n");
            exit(EXIT_FAILURE);
        }
        lock->holder = waiter;
    } else {
        /* The waiter stuct for the fiber already exists, check if it's still banned. */
        struct timeval cur_time;
        gettimeofday(&cur_time, NULL);

        if (timercmp(&waiter->end_ticks, &waiter->banned_until, <) &&
            timercmp(&cur_time, &waiter->banned_until, <)) {
            /* Banned => let us serve other threads*/
            atomic_fetch_add(&lock->now_serving, 1);

            do {
                gettimeofday(&cur_time, NULL);
            } while (timercmp(&cur_time, &waiter->banned_until, <));

            /* Re-acquire a new ticket once ban is lifted. */
            my_ticket = atomic_fetch_add(&lock->next_ticket, 1);
            while (atomic_load(&lock->now_serving) != my_ticket) { // spin while not being served 
            }
        }
        /* Ban time has been served so we can get the lock */
        gettimeofday(&waiter->start_ticks, NULL);
        lock->holder = waiter;
    }
}

void fair_unlock(struct fairlock *lock)
{
    struct fairlock_waiter *waiter= lock->holder;
    struct fairlock_waiter *prev_waiter, *tmp;
    unsigned int num_threads;
    unsigned long long cs_length;
    struct timeval now;
    struct timeval time_adder = {0, 0};

    gettimeofday(&now, NULL);
    waiter->end_ticks = now;
    num_threads = atomic_load(&lock->num_threads);
    if (num_threads > 1) {
        /* Expand ban time by (cs_length * num_threads). */
        cs_length = time_diff(waiter->start_ticks, now);
        time_adder.tv_sec  = (cs_length * num_threads) / 1000000ULL;
        time_adder.tv_usec = (cs_length * num_threads) % 1000000ULL;

        timeval_add(&waiter->banned_until, &waiter->banned_until, &time_adder);

        /*
         * Clean up inactive waiters if they've been idle longer than
         */
        list_for_each_entry_safe_reverse(prev_waiter, tmp, &waiter->list, list) {
            /* If we reached the list head, break. */
            if (&prev_waiter->list == &lock->waiters)
                continue;

            /* Inactive threshold: now - 1 second. */
            inactive_threshold.tv_sec  = now.tv_sec - 1;
            inactive_threshold.tv_usec = now.tv_usec;

            if (timercmp(&prev_waiter->end_ticks, &inactive_threshold, <)) {
                /* Remove from list & hashtable, then free. */
                list_del(&prev_waiter->list);
                hash_del(&prev_waiter->hash);
                free(prev_waiter);
                atomic_fetch_sub(&lock->num_threads, 1);
            }
        }
    } else {
        /* If only one fiber, no ban needed. */
        waiter->banned_until = now;
    }
    /* Move to next waiter */
    atomic_fetch_add(&lock->now_serving, 1);
}
