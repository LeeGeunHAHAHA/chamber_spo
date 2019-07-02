/**
 * @file list.h
 * @date 2016. 05. 16.
 * @author kmsun
 *
 * list implementation from Linux kernel.
 */
#ifndef __NBIO_LIST_H__
#define __NBIO_LIST_H__

#include <stddef.h>

#define POISON_POINTER_DELTA    (0xdead000000000000)
#define LIST_POISON1            ((void *) 0x100 + POISON_POINTER_DELTA)
#define LIST_POISON2            ((void *) 0x200 + POISON_POINTER_DELTA)

struct list_head {
    struct list_head *next, *prev;
};

#define LIST_HEAD_INIT(name)    { &(name), &(name) }

#define LIST_HEAD(name) \
    struct list_head name = LIST_HEAD_INIT(name)

static inline void INIT_LIST_HEAD(struct list_head *list)
{
    list->next = list;
    list->prev = list;
}

static inline void __list_add(struct list_head *new,
                              struct list_head *prev,
                              struct list_head *next)
{
    next->prev = new;
    new->next = next;
    new->prev = prev;
    prev->next = new;
}

static inline void list_add(struct list_head *new, struct list_head *head)
{
    __list_add(new, head, head->next);
}

static inline void list_add_tail(struct list_head *new, struct list_head *head)
{
    __list_add(new, head->prev, head);
}

static inline void __list_del(struct list_head *prev, struct list_head *next)
{
    next->prev = prev;
    prev->next = next;
}

static inline void __list_del_entry(struct list_head *entry)
{
    __list_del(entry->prev, entry->next);
}

static inline void list_del(struct list_head *entry)
{
    __list_del(entry->prev, entry->next);
    entry->next = LIST_POISON1;
    entry->prev = LIST_POISON2;
}

static inline int list_empty(const struct list_head *head)
{
    return head->next == head;
}

#define list_entry(ptr, type, member) \
    container_of(ptr, type, member)
#define list_first_entry(ptr, type, member) \
    list_entry((ptr)->next, type, member)
#define list_last_entry(ptr, type, member) \
    list_entry((ptr)->prev, type, member)
#define list_first_entry_or_null(ptr, type, member) \
    (!list_empty(ptr) ? list_first_entry(ptr, type, member) : NULL)
#define list_next_entry(pos, member) \
    list_entry((pos)->member.next, typeof(*(pos)), member)
#define list_prev_entry(pos, member) \
    list_entry((pos)->member.prev, typeof(*(pos)), member)
#define list_for_each(pos, head) \
    for (pos = (head)->next; pos != (head); pos = pos->next)
#define list_for_each_prev(pos, head) \
    for (pos = (head)->prev; pos != (head); pos = pos->prev)
#define list_for_each_safe(pos, n, head) \
    for (pos = (head)->next, n = pos->next; pos != (head); \
            pos = n, n = pos->next)
#define list_for_each_prev_safe(pos, n, head) \
    for (pos = (head)->prev, n = pos->prev; \
            pos != (head); \
            pos = n, n = pos->prev)
#define list_for_each_entry(pos, head, member)              \
    for (pos = list_first_entry(head, typeof(*pos), member);    \
            &pos->member != (head);                    \
            pos = list_next_entry(pos, member))
#define list_for_each_entry_reverse(pos, head, member)          \
    for (pos = list_last_entry(head, typeof(*pos), member);     \
            &pos->member != (head);                    \
            pos = list_prev_entry(pos, member))
#define list_prepare_entry(pos, head, member) \
    ((pos) ? : list_entry(head, typeof(*pos), member))
#define list_for_each_entry_continue(pos, head, member)         \
    for (pos = list_next_entry(pos, member);            \
            &pos->member != (head);                    \
            pos = list_next_entry(pos, member))
#define list_for_each_entry_continue_reverse(pos, head, member)     \
    for (pos = list_prev_entry(pos, member);            \
            &pos->member != (head);                    \
            pos = list_prev_entry(pos, member))
#define list_for_each_entry_from(pos, head, member)             \
    for (; &pos->member != (head);                  \
            pos = list_next_entry(pos, member))
#define list_for_each_entry_safe(pos, n, head, member)          \
    for (pos = list_first_entry(head, typeof(*pos), member),    \
            n = list_next_entry(pos, member);           \
            &pos->member != (head);                    \
            pos = n, n = list_next_entry(n, member))
#define list_for_each_entry_safe_continue(pos, n, head, member)         \
    for (pos = list_next_entry(pos, member),                \
            n = list_next_entry(pos, member);               \
            &pos->member != (head);                        \
            pos = n, n = list_next_entry(n, member))
#define list_for_each_entry_safe_from(pos, n, head, member)             \
    for (n = list_next_entry(pos, member);                  \
            &pos->member != (head);                        \
            pos = n, n = list_next_entry(n, member))
#define list_for_each_entry_safe_reverse(pos, n, head, member)      \
    for (pos = list_last_entry(head, typeof(*pos), member),     \
            n = list_prev_entry(pos, member);           \
            &pos->member != (head);                    \
            pos = n, n = list_prev_entry(n, member))
#define list_safe_reset_next(pos, n, member)                \
    n = list_next_entry(pos, member)

#endif /* __NBIO_LIST_H__ */

