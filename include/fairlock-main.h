#include <fairlock.h>
#include <timing.h>
#include <fiber_manager.h>

/*
 * This value will change depending on the CPU-SPEED.
 * Please set the value accordingly.
 */
#define INACTIVE_THRESHOLD 1 /* 1 sec */
struct timeval inactive_threshold;

struct fairlock_waiter {
	struct timeval banned_until;
	struct timeval start_ticks;
	struct timeval end_ticks;
	struct hlist_node hash;
	struct list_head list;
	int fid;
};

inline struct fairlock_waiter *create_waiter(struct fairlock *lock, int fid_c)
{
	struct timeval now;
	struct fairlock_waiter *waiter;

	gettimeofday(&now, NULL);
	waiter = malloc(sizeof(struct fairlock_waiter));
	if (!waiter) {
		return waiter;
	}
	waiter->fid = fid_c;
	waiter->banned_until = now;
	waiter->start_ticks = now;
	waiter->end_ticks = now;
	INIT_LIST_HEAD(&waiter->list);
	list_add_tail(&waiter->list, &lock->waiters);
	INIT_HLIST_NODE(&waiter->hash);
	hash_add(lock->waiters_lookup, &waiter->hash, fid_c);
	atomic_inc(&lock->num_threads);
	return waiter;
}

inline struct fairlock_waiter *retrieve_waiter(struct fairlock *lock, int fid)
{
	struct fairlock_waiter *waiter;

	hash_for_each_possible(lock->waiters_lookup, waiter, hash, fid) {
		if (waiter->fid == fid)
			return waiter;
	}
	return NULL;
}

inline void fairlock_init(struct fairlock *lock)
{
	hash_init(lock->waiters_lookup);
	INIT_LIST_HEAD(&lock->waiters);
	lock->num_threads = (atomic_t) ATOMIC_INIT(0);
	lock->next_ticket = (atomic_t) ATOMIC_INIT(0);
	lock->now_serving = (atomic_t) ATOMIC_INIT(0);
}
// EXPORT_SYMBOL(fairlock_init);

inline void fairlock_destroy(struct fairlock *lock)
{
	unsigned int end_ticket;

	end_ticket = atomic_fetch_inc(&lock->next_ticket);
	while (atomic_read(&lock->now_serving) != end_ticket);
}

int fair_trylock(struct fairlock *lock, int fid)
{
	unsigned int my_ticket;
	unsigned int serving;
	struct fairlock_waiter *waiter;

	serving = atomic_read(&lock->now_serving);
	if (serving != atomic_cmpxchg(&lock->next_ticket, serving, serving + 1))
		return 0;
	my_ticket = serving;

	waiter = retrieve_waiter(lock, fid);

	if (!waiter) {
		waiter = create_waiter(lock, fid);
		if (!waiter) {
			return 0;
		}
	} else {
		timeval cur_time;
		gettimeofday(&cur_time, NULL);
		if (timercmp(&waiter->end_ticks, &waiter->banned_until, <) && 
    timercmp(&curtime, &waiter->banned_until, <))
		{
			atomic_inc(&lock->now_serving);
			return 0;
		}
		gettimeofday(&waiter->start_ticks, NULL);
	}
	lock->holder = waiter;
	return 1;
}


void fair_lock(struct fairlock *lock, int fid)
{
	unsigned int my_ticket;
	struct fairlock_waiter *waiter;

	my_ticket = atomic_fetch_inc(&lock->next_ticket);
	while (atomic_read(&lock->now_serving) != my_ticket) {
		fiber_yield();
	}

	waiter = retrieve_waiter(lock, fid);

	if (!waiter) {
		waiter = create_waiter(lock, fid);
		if (!waiter) {
			panic("Unable to allocate memory for fairlock waiter\n");
		}
		lock->holder = waiter;
	} else {
		timeval cur_time;
		gettimeofday(&cur_time, NULL);
		if (timercmp(&waiter->end_ticks, &waiter->banned_until, <) && timercmp(&curtime, &waiter->banned_until, <)) {
			atomic_inc(&lock->now_serving);
			do {
				fiber_yield();
				gettimeofday(&cur_time, NULL);
			} while (timercmp(&cur_time, &waiter->banned_until, <));
			my_ticket = atomic_fetch_inc(&lock->next_ticket);
			while (atomic_read(&lock->now_serving) != my_ticket);
		}
		gettimeofday(&waiter->start_ticks, NULL);
		lock->holder = waiter;
	}
}


void fair_unlock(struct fairlock *lock)
{
	struct fairlock_waiter *waiter, *prev_waiter, *tmp;
	unsigned int num_threads;
	unsigned long long cs_length;
	struct timeval now;
	struct timeval time_adder;

	waiter = lock->holder;
	gettimeofday(&now, NULL);
	waiter->end_ticks = now;
	num_threads = atomic_read(&lock->num_threads);
	if (num_threads > 1) {
		cs_length =time_diff(waiter->start_ticks, now);
		time_adder.tv_usec = (cs_length*num_threads)%1000000
		time_added.tv_sec = (cs_length*num_threads)/1000000

		timeval_add(&waiter->banned_until, &waiter->banned_until, &time_adder)

		list_for_each_entry_safe_reverse(prev_waiter, tmp, &waiter->list, list) {
			if (&prev_waiter->list == &lock->waiters)
				continue;
			inactive_threshold.tv_sec = now.tv_sec - 1;
			inactive_threshold.tv_usec = now.tv_usec ;
			if timercmp(&prev_waiter->end_ticks, &inactive_threshold, <)
			{
				list_del(&prev_waiter->list);
				hash_del(&prev_waiter->hash);
				free(prev_waiter);
				atomic_dec(&lock->num_threads);
			}
		}
	} else {
		waiter->banned_until = now;
	}
	atomic_inc(&lock->now_serving);
}
