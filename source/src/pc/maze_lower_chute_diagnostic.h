#ifndef MELEE_PC_MAZE_LOWER_CHUTE_DIAGNOSTIC_H
#define MELEE_PC_MAZE_LOWER_CHUTE_DIAGNOSTIC_H

/* Opt-in PAD-only route from the third sword to authored lower floor46.
 * This header is included after the existing fighter/controller definitions.
 * Never assign collision, actor, encounter, stock, damage or RNG state. */
static void MeleePc_MazeLowerChuteInput(Fighter* fp, PADStatus* input,
                                       unsigned frame)
{
    static unsigned start;
    static int stocks, phase, done, aborted;
    int floor = fp->coll_data.floor.index;
    if (!start) {
        start = frame;
        stocks = Player_GetStocks(0);
        OSReport("NATIVE_MAZE_LOWER_CHUTE_START frame=%u x=%f y=%f floor=%d stocks=%d damage=%d\n",
                 frame, fp->cur_pos.x, fp->cur_pos.y, floor, stocks,
                 Player_GetDamage(0));
    }
    if (done || aborted) return;
    if (Player_GetStocks(0) != stocks || frame - start > 1200 ||
        fp->cur_pos.y < -270 || fp->cur_pos.x < -335 || fp->cur_pos.x > 175) {
        aborted = 1;
        OSReport("NATIVE_MAZE_LOWER_CHUTE_ABORT frame=%u phase=%d x=%f y=%f floor=%d stocks=%d\n",
                 frame, phase, fp->cur_pos.x, fp->cur_pos.y, floor,
                 Player_GetStocks(0));
        return;
    }
    if (frame - start < 24) return;
    if (fp->motion_id == ftCo_MS_CaptureLeadead ||
        fp->motion_id == ftCo_MS_CaptureLikelike) {
        input->button = frame % 8 < 2 ? PAD_BUTTON_A : 0;
    } else if (fp->motion_id == ftCo_MS_DownWaitU ||
               fp->motion_id == ftCo_MS_DownWaitD) {
        input->button = frame % 8 < 2 ? PAD_TRIGGER_L : 0;
    } else if (!phase) {
        /* Connected floors15 ->36 ->35 ->34 ->33 end at x=-201.97.
         * Walk off normally; strong stick exits the original teeter state. */
        input->stickX = fp->motion_id == 245 || fp->motion_id == 246 ? -80 : -45;
        if (fp->ground_or_air != GA_Ground && fp->cur_pos.x < -204) {
            phase = 1;
            OSReport("NATIVE_MAZE_LOWER_CHUTE_EDGE frame=%u x=%f y=%f floor=%d\n",
                     frame, fp->cur_pos.x, fp->cur_pos.y, floor);
        }
    } else {
        /* The authored outer/inner walls bend west with decreasing height.
         * These steering targets stay inside that chute, not through walls. */
        float y = fp->cur_pos.y;
        float target = y > 35 ? -218 : y > 0 ? -223 :
                       y > -30 ? -239 : y > -90 ? -253 :
                       y > -180 ? -264 : -246;
        float projected = fp->cur_pos.x + 6.0f * fp->self_vel.x;
        input->stickX = projected < target - 2 ? 55 :
                        projected > target + 2 ? -55 : 0;
        if (fp->ground_or_air == GA_Ground &&
            (floor == 31 || floor == 32 || floor == 47 || floor == 48)) {
            /* Repeated neutral/down edges use original platform-pass IASA. */
            input->stickX = 0;
            input->stickY = (frame - start) % 24 < 10 ? -100 : 0;
        }
        if (fp->ground_or_air == GA_Ground && floor == 46 &&
            fp->cur_pos.x > -280 && fp->cur_pos.x < -170 &&
            y > -240 && y < -235) {
            done = 1;
            memset(input, 0, sizeof(*input));
            OSReport("NATIVE_MAZE_LOWER_CHUTE_LANDED frame=%u x=%f y=%f floor=%d stocks=%d damage=%d\n",
                     frame, fp->cur_pos.x, y, floor, Player_GetStocks(0),
                     Player_GetDamage(0));
        }
    }
    if (frame % 5 == 0)
        OSReport("NATIVE_MAZE_LOWER_CHUTE frame=%u phase=%d x=%f y=%f vx=%f floor=%d air=%d motion=%d requested_x=%d requested_y=%d button=%u done=%d aborted=%d\n",
                 frame, phase, fp->cur_pos.x, fp->cur_pos.y, fp->self_vel.x,
                 floor, fp->ground_or_air, fp->motion_id, input->stickX,
                 input->stickY, input->button, done, aborted);
}

#endif
