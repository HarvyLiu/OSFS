// ticket.c -- fairness in 10 lines. Lesson docs/en.md.
#include <stdatomic.h>
#include "ticket.h"

void ticket_init(ticket_t *l) {
    atomic_store(&l->next_ticket, 0);
    atomic_store(&l->now_serving, 0);
}

void ticket_lock(ticket_t *l) {
    unsigned my = atomic_fetch_add(&l->next_ticket, 1);
    while (atomic_load(&l->now_serving) != my) {
#if defined(__x86_64__) || defined(_M_X64)
        __asm__ volatile ("pause");
#endif
    }
}

void ticket_unlock(ticket_t *l) {
    atomic_fetch_add(&l->now_serving, 1);
}
