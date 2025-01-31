#ifndef __HASHMAP_H__
#define __HASHMAP_H__

#include <stddef.h>  /* for size_t, NULL */
#include <stdint.h>  /* for standard int types */
#include <stdlib.h>  /* for malloc/free */
#include <string.h>  /* for memset */

/*
 * Simple doubly-linked 'hlist' for each bucket.
 * We only need next/pprev in this minimal version.
 */
struct hlist_node {
    struct hlist_node *next, **pprev;
};

struct hlist_head {
    struct hlist_node *first;
};

static inline void INIT_HLIST_NODE(struct hlist_node *h)
{
    h->next  = NULL;
    h->pprev = NULL;
}

static inline void INIT_HLIST_HEAD(struct hlist_head *h)
    {
        h->first = NULL;
}

/* A container_of() macro like the Linux kernel */
#ifndef container_of
#define container_of(ptr, type, member) \
    ((type *)((char *)(ptr) - (unsigned long)(&((type *)0)->member)))
#endif

/* Initialize all buckets in a hashtable to empty. */
#define hash_init(ht) do {                                          \
    size_t __i, __size = (sizeof(ht) / sizeof((ht)[0]));            \
    for (__i = 0; __i < __size; __i++) {                            \
        INIT_HLIST_HEAD(&(ht)[__i]);                                \
    }                                                               \
} while (0)

/*
 * Basic hash function that masks the lower bits of 'key'
 * according to 'bits'.
 */
static inline unsigned int _hash_key(unsigned int key, unsigned int bits)
{
    return key & ((1U << bits) - 1U);
}

/* Insert a node at the head of an hlist bucket */
static inline void hlist_add_head(struct hlist_node *n, struct hlist_head *head)
{
    struct hlist_node *first = head->first;
    n->next = first;
    if (first)
        first->pprev = &n->next;
    head->first = n;
    n->pprev = &head->first;
}

/* Remove a node from whichever bucket it belongs to */
static inline void hlist_del(struct hlist_node *n)
{
    struct hlist_node *next = n->next;
    struct hlist_node **pprev = n->pprev;

    if (pprev) {
        *pprev = next;
        if (next)
            next->pprev = pprev;
        n->next = NULL;
        n->pprev = NULL;
    }
}

/*
 * hash_add: Add 'node' to the bucket for 'key'.
 * bits=8 means 256 buckets, so index = key & 255.
 */
#define hash_add(ht, node, key) do {                             \
    unsigned int __index = _hash_key((key), 8 /* or your bits */); \
    hlist_add_head((node), &(ht)[__index]);                      \
} while (0)

/*
 * hash_del: remove a node from the hashtable
 */
#define hash_del(node) hlist_del(node)

/*
 * hash_for_each_possible: iterate over all nodes in the bucket for 'key'.
 *
 * Usage:
 *   struct fairlock_waiter *waiter;
 *   hash_for_each_possible(lock->waiters_lookup, waiter, hash, fid) {
 *       if (waiter->fid == fid)
 *           return waiter;
 *   }
 *
 * Where:
 *   - (ht)  : your hashtable array
 *   - (obj) : loop cursor of type pointer (e.g. struct fairlock_waiter *)
 *   - (member) : the hlist_node field name in your struct
 *   - (key) : integer key
 */
#define hash_for_each_possible(ht, obj, member, key)                               \
    for (struct hlist_node *__pos = (ht)[_hash_key((key), 8)].first; __pos != NULL; \
         __pos = __pos->next)                                                     \
        if (((obj) = container_of(__pos, typeof(*(obj)), member)), true)

#endif /* __HASHMAP_H__ */
