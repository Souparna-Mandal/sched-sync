#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "fiber_manager.h"
#ifdef FAIRLOCK
    #include "fairlock-main2.h"
#endif
#ifdef SCHEDLOCK
    #include "schedlock.h"
#endif
// #include"timing.h"

typedef unsigned long long ull;
typedef struct timespec timespec_t;
int nthreads;

typedef struct {
    int id;
    ull cs;
    ull num_lock_acquired;
    ull loop_count_in_cs;
    ull lock_hold_time;
    struct timeval start_time;
    ull duration;
} task_t;

#ifdef FAIRLOCK
        // TODO: Implement FAIRLOCK logic here
    struct fairlock lock;
#endif
#ifdef MUTEX
    fiber_mutex_t mutex;
#endif
#ifdef SCHEDLOCK
        // TODO: Implement FAIRLOCK logic here
    struct sched_lock lock;
#endif


void* run_func(void* param) {
    task_t *task = (task_t *)param;

    struct timeval now, start;
    ull lock_acquires = 0;
    ull lock_hold = 0;
    ull loop_in_cs = 0;

    gettimeofday(&now, NULL);

    while (time_diff(task->start_time, now) < task->duration * 1000000) {
#ifdef FAIRLOCK
        // TODO: Implement FAIRLOCK logic here
        fair_lock(&lock, task->id);
#endif
#ifdef MUTEX
        fiber_mutex_lock(&mutex);
#endif
#ifdef SCHEDLOCK
        sched_lock_acquire(&lock);
#endif

        gettimeofday(&start, NULL);
        lock_acquires++;

        do {
            loop_in_cs++;
            gettimeofday(&now, NULL);
        } while (time_diff(start, now) < task->cs);

        lock_hold += time_diff(start, now);

#ifdef FAIRLOCK
        // TODO: Implement FAIRLOCK logic here
        fair_unlock(&lock);
#endif
#ifdef MUTEX
        fiber_mutex_unlock(&mutex);
#endif
#ifdef SCHEDLOCK
        sched_lock_release(&lock, nthreads);
#endif

        gettimeofday(&now, NULL);
    }

    task->num_lock_acquired = lock_acquires;
    task->loop_count_in_cs = loop_in_cs;
    task->lock_hold_time = lock_hold;

    printf("id %02d "
           "loop %10llu "
           "lock_acquires %8llu "
           "lock_hold(us) %10llu\n",
           task->id,
           task->loop_count_in_cs,
           task->num_lock_acquired,
           task->lock_hold_time);

    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("usage: %s <nthreads> <duration> <critical section 1> <critical section 2> ...\n", argv[0]);
        printf("nthreads - number of threads\n");
        printf("duration - duration of the experiment in seconds (s)\n");
        printf("critical section - critical section size in microseconds (us)\n");
        return 1;
    }

    nthreads = atoi(argv[1]);

    ull duration = atoll(argv[2]);

    fiber_manager_init(nthreads/2);
    task_t tasks[nthreads];
    fiber_t* fibers[nthreads];

    struct timeval start_time;
    gettimeofday(&start_time, NULL);

    for (int i = 0; i < nthreads; i++) {
        tasks[i].id = i;
        tasks[i].cs = atoll(argv[3 + i]);
        tasks[i].num_lock_acquired = 0;
        tasks[i].loop_count_in_cs = 0;
        tasks[i].lock_hold_time = 0;
        tasks[i].start_time = start_time;
        tasks[i].duration = duration;
    }

#ifdef FAIRLOCK
    // TODO: Initialize FAIRLOCK here
    fairlock_init(&lock);

#endif
#ifdef MUTEX
    fiber_mutex_init(&mutex);
#endif
#ifdef SCHEDLOCK
    sched_lock_init(&lock);
#endif

    for (int i = 0; i < nthreads; i++) {
        fibers[i] = fiber_create(10240, &run_func, (void*)&tasks[i]);
    }

    for (int i = 0; i < nthreads; i++) {
        fiber_join(fibers[i], NULL);
    }

#ifdef MUTEX
    fiber_mutex_destroy(&mutex);
#endif

    // fiber_manager_print_stats();
    fiber_shutdown();

    return 0;
}
