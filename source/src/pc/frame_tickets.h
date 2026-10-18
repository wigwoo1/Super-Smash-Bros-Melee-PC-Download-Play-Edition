#ifndef MELEE_PC_FRAME_TICKETS_H
#define MELEE_PC_FRAME_TICKETS_H
#include <stdint.h>
#define MELEE_PC_FRAME_TICKET_CAPACITY 3
typedef struct {
    uint64_t id;
    unsigned active, completed, presented, failed;
} MeleePc_FrameTicket;
typedef struct {
    uint64_t generation;
    uint64_t last_submitted_id;
    MeleePc_FrameTicket slots[MELEE_PC_FRAME_TICKET_CAPACITY];
} MeleePc_FrameTickets;
/* Zero-initialize; owner-thread only. Reset refuses pending tickets and a
 * reused generation. These are acknowledgement tickets, not HSD XFB states. */
int MeleePc_FrameTicketsReset(MeleePc_FrameTickets* tickets, uint64_t generation);
int MeleePc_FrameTicketSubmit(MeleePc_FrameTickets* tickets, uint64_t generation,
                             uint64_t id);
/* kind1=GPU completion, kind2=presentation acceptance; status0=success.
 * Failed completion does not authorize resource reuse or ordinary reset. */
int MeleePc_FrameTicketAck(MeleePc_FrameTickets* tickets, uint64_t generation,
                          uint64_t id, unsigned kind, unsigned status);
int MeleePc_FrameTicketRetire(MeleePc_FrameTickets* tickets, uint64_t generation,
                             uint64_t id);
#endif
