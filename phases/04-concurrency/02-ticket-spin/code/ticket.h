// ticket.h -- deli-counter shape. Lesson docs/en.md.
#ifndef OSFS_TICKET_H
#define OSFS_TICKET_H

typedef struct { _Atomic unsigned next_ticket; _Atomic unsigned now_serving; } ticket_t;
void ticket_init(ticket_t *l);
void ticket_lock(ticket_t *l);
void ticket_unlock(ticket_t *l);

#endif
