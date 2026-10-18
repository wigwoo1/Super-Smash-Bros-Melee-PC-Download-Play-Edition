#include "frame_tickets.h"
#include <string.h>
static MeleePc_FrameTicket* find(MeleePc_FrameTickets* tickets,
                                uint64_t generation, uint64_t id)
{
    if (!tickets || !generation || generation != tickets->generation || !id)
        return 0;
    for (unsigned i = 0; i < MELEE_PC_FRAME_TICKET_CAPACITY; i++)
        if (tickets->slots[i].active && tickets->slots[i].id == id)
            return &tickets->slots[i];
    return 0;
}
int MeleePc_FrameTicketsReset(MeleePc_FrameTickets* tickets, uint64_t generation)
{
    if (!tickets || generation <= tickets->generation) return 0;
    for (unsigned i = 0; i < MELEE_PC_FRAME_TICKET_CAPACITY; i++)
        if (tickets->slots[i].active) return 0;
    memset(tickets, 0, sizeof(*tickets));
    tickets->generation = generation;
    return 1;
}
int MeleePc_FrameTicketSubmit(MeleePc_FrameTickets* tickets, uint64_t generation,
                             uint64_t id)
{
    if (!tickets || !generation || generation != tickets->generation || !id ||
        id <= tickets->last_submitted_id ||
        find(tickets, generation, id)) return 0;
    for (unsigned i = 0; i < MELEE_PC_FRAME_TICKET_CAPACITY; i++) {
        if (!tickets->slots[i].active) {
            tickets->slots[i] = (MeleePc_FrameTicket){id, 1, 0, 0, 0};
            tickets->last_submitted_id = id;
            return 1;
        }
    }
    return 0;
}
int MeleePc_FrameTicketAck(MeleePc_FrameTickets* tickets, uint64_t generation,
                          uint64_t id, unsigned kind, unsigned status)
{
    MeleePc_FrameTicket* slot = find(tickets, generation, id);
    if (!slot || status > 1) return 0;
    if (kind == 1 && !slot->completed) {
        slot->completed = 1;
        slot->failed |= status;
        return 1;
    }
    if (kind == 2 && !slot->presented) {
        slot->presented = 1;
        slot->failed |= status;
        return 1;
    }
    return 0;
}
int MeleePc_FrameTicketRetire(MeleePc_FrameTickets* tickets, uint64_t generation,
                             uint64_t id)
{
    MeleePc_FrameTicket* slot = find(tickets, generation, id);
    if (!slot || !slot->completed || !slot->presented || slot->failed) return 0;
    memset(slot, 0, sizeof(*slot));
    return 1;
}
