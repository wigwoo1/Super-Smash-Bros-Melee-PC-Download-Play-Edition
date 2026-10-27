#ifndef MELEE_PC_SIGNED_WORD_DIVISION_H
#define MELEE_PC_SIGNED_WORD_DIVISION_H
#include <limits.h>
/* Gekko signed-word quotient, including non-trapping exceptional operands.
 * Reference: Dolphin Interpreter_Integer.cpp, Interpreter::divwx.
 * This helper supplies the value only: this caller does not consume XER/CR. */
static inline int MeleePc_DivideSignedWord(int dividend, int divisor)
{
    if (divisor == 0) {
        return dividend < 0 ? -1 : 0;
    }
    if (dividend == INT_MIN && divisor == -1) {
        return -1;
    }
    return dividend / divisor;
}
#endif
