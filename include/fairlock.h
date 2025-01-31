#ifndef __LINUX_FAIRLOCK_H
#define __LINUX_FAIRLOCK_H

#include <stdatomic.h>
#include "hashmap.h"
#include "list.h"

struct fairlock_waiter;

struct fairlock {
    // DECLARE_HASHTABLE(waiters_lookup, 8);
    struct hlist_head waiters_lookup[(1 << 8)]; // there will be 8 bits so 256 buckets
    struct list_head waiters;
    atomic_int num_threads;
    atomic_int next_ticket;
    atomic_int now_serving;
    struct fairlock_waiter *holder;
};

extern void fairlock_init(struct fairlock *lock);
extern void fairlock_destroy(struct fairlock *lock);
extern int fair_trylock(struct fairlock *lock, int fid);
extern void fair_lock(struct fairlock *lock, int fid);
extern void fair_unlock(struct fairlock *lock);

#endif /* __LINUX_FAIRLOCK_H */
