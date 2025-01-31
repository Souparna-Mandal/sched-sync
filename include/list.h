#ifndef __LIST_H__
#define __LIST_H__

#include <stddef.h> /* for container_of if you define it, etc. */

struct list_head {
    struct list_head *next, *prev;
};

/* Initialize a list head to point to itself */
static inline void INIT_LIST_HEAD(struct list_head *list)
{
    list->next = list;
    list->prev = list;
}

/* Internal helper to link a new entry between two known consecutive entries */
static inline void __list_add(struct list_head *new_entry,
                              struct list_head *prev,
                              struct list_head *next)
{
    next->prev      = new_entry;
    new_entry->next = next;
    new_entry->prev = prev;
    prev->next      = new_entry;
}

/* Add new entry at the tail of the list */
static inline void list_add_tail(struct list_head *new_entry,
                                 struct list_head *head)
{
    __list_add(new_entry, head->prev, head);
}

/* Internal helper to unlink an entry */
static inline void __list_del(struct list_head *prev,
                              struct list_head *next)
{
    next->prev = prev;
    prev->next = next;
}

/* Delete an entry from the list */
static inline void list_del(struct list_head *entry)
{
    __list_del(entry->prev, entry->next);
    entry->next = entry->prev = NULL;
}

/* Return pointer to struct that 'member' is embedded in */
#ifndef container_of
#define container_of(ptr, type, member) \
    ((type *)((char *)(ptr) - (unsigned long)(&((type *)0)->member)))
#endif

/* Return the struct for this entry, given the head pointer */
#define list_last_entry(head, type, member) \
    container_of((head)->prev, type, member)

/* Return the previous list entry for an element */
#define list_prev_entry(pos, member) \
    container_of((pos)->member.prev, typeof(*(pos)), member)

/*
 * list_for_each_entry_safe_reverse(pos, n, head, member)
 *
 * Safely iterate backwards over the list from tail to head,
 * allowing the removal of 'pos' from the list during iteration.
 */
#define list_for_each_entry_safe_reverse(pos, n, head, member)                \
    for (pos = list_last_entry(head, typeof(*pos), member),                   \
         n   = list_prev_entry(pos, member);                                  \
         &pos->member != (head);                                              \
         pos = n, n = list_prev_entry(n, member))

#endif /* __LIST_H__ */
