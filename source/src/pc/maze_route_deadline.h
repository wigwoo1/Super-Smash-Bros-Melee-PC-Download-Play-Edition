#ifndef MELEE_PC_MAZE_ROUTE_DEADLINE_H
#define MELEE_PC_MAZE_ROUTE_DEADLINE_H

/* Diagnostic PAD route only: 900 approach ticks, at most 120 extra ticks
 * for an already accepted jump. A new attempt never moves the hard cap. */
static int MeleePc_MazeUpperExpired(unsigned total_ticks, int jump_active,
                                   unsigned jump_ticks)
{
    return total_ticks > 1020 ||
           (total_ticks > 900 && (!jump_active || jump_ticks > 120));
}

#endif
