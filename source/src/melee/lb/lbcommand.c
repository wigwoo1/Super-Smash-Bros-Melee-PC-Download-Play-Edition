#include "lbcommand.h"

#include "inlines.h"
#include "lb_0219.h"
#include "types.h"
#ifdef TARGET_PC
#include <sysdolphin/baselib/debug.h>
#endif

void (*lbCommand_803B9840[16])(CommandInfo*) = {
    Command_00, Command_01, Command_02, Command_03, Command_04, Command_05,
    Command_06, Command_07, Command_08, Command_09, NULL,       NULL,
    NULL,       NULL,       NULL,       NULL
};

/// Reset
void Command_00(CommandInfo* info)
{
    info->x8.u = NULL;
}

/// SynchronousTimer
void Command_01(CommandInfo* info)
{
    info->timer += info->x8.u->Command_00.value;
    NEXT_CMD(info);
}

/// AsynchronousTimer
void Command_02(CommandInfo* info)
{
    info->timer = info->x8.u->Command_02.value - info->frame_count;
    NEXT_CMD(info);
}

/// SetLoop
void Command_03(CommandInfo* info)
{
    info->event_return[info->loop_count++] = info->x8.u + 1;
    info->event_return[info->loop_count++] =
        (union CmdUnion*) info->x8.u->Command_03.value;
    NEXT_CMD(info);
}

/// Execute Loop
void Command_04(CommandInfo* info)
{
    u32* ptr = (u32*) info;
    ptr[info->loop_count + 3] -= 1;

    if ((s32) info->event_return[info->loop_count - 1]) {
        info->x8.ptr[0] = &info->x8.ptr[info->loop_count][0];
        return;
    }
    NEXT_CMD(info);
    info->loop_count -= 2;
}

/// Subroutine
void Command_05(CommandInfo* info)
{
    NEXT_CMD(info);
    info->event_return[info->loop_count++] = info->x8.u + 1;
    info->x8.u = info->x8.u->Command_05.ptr;
}

/// Return
void Command_06(CommandInfo* info)
{
    info->x8.u = info->event_return[info->loop_count -= 1];
}

/// Goto
void Command_07(CommandInfo* info)
{
    NEXT_CMD(info);
    info->x8.u = info->x8.u->Command_07.ptr;
}

/// SetTimerAnimation
void Command_08(CommandInfo* info)
{
    NEXT_CMD(info);
    info->timer = F32_MAX;
}

void Command_09(CommandInfo* info)
{
    lbBgFlash_80021C48(info->x8.u->Command_09.param_1,
                       info->x8.u->Command_09.param_2);
    NEXT_CMD(info);
}

bool Command_Execute(CommandInfo* info, u32 command)
{
    if (command < 10) {
        lbCommand_803B9840[command](info);
        return true;
    }
    return false;
}

#ifdef TARGET_PC
/* Packed item command words remain BE; relocated call/goto pointer words are
 * native. The remaining shared handlers only use cursor/stack operations. */
bool Command_ExecutePackedBe(CommandInfo* info, u32 command)
{
    const u8* bytes;
    u32 word, value;
    if (command >= 10) {
        return false;
    }
    HSD_ASSERT(0, info && info->x8.u);
    bytes = (const u8*) info->x8.u;
    word = ((u32) bytes[0] << 24) | ((u32) bytes[1] << 16) |
           ((u32) bytes[2] << 8) | bytes[3];
    HSD_ASSERT(0, (word >> 26) == command);
    value = word & 0x3FFFFFF;
    switch (command) {
    case 1:
        info->timer += value;
        NEXT_CMD(info);
        break;
    case 2:
        info->timer = value - info->frame_count;
        NEXT_CMD(info);
        break;
    case 3:
        HSD_ASSERT(0, info->loop_count <= ARRAY_SIZE(info->event_return) - 2);
        info->event_return[info->loop_count++] = info->x8.u + 1;
        info->event_return[info->loop_count++] = (union CmdUnion*) value;
        NEXT_CMD(info);
        break;
    case 9:
        lbBgFlash_80021C48((word >> 18) & 255, word & 0x3FFFF);
        NEXT_CMD(info);
        break;
    default:
        if (command == 4) { HSD_ASSERT(0, info->loop_count >= 2); }
        if (command == 5) {
            HSD_ASSERT(0, info->loop_count < ARRAY_SIZE(info->event_return));
        }
        if (command == 6) { HSD_ASSERT(0, info->loop_count >= 1); }
        lbCommand_803B9840[command](info);
        break;
    }
    return true;
}
#endif
