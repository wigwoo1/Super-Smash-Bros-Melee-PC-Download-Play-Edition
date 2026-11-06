#ifndef MELEE_PC_P2_CONTACT_DIAGNOSTIC_H
#define MELEE_PC_P2_CONTACT_DIAGNOSTIC_H

/* Opt-in controller-only probe. Fighter positions, physics, damage, hitboxes,
 * player type and match outcomes remain read-only. Use the original PAD latch,
 * attack selection and collision paths; P1 stays neutral to exclude self-hits. */
static void MeleePc_Player2ContactInput(PADStatus* p1, unsigned frame)
{
    PADStatus p2 = { 0 };
    HSD_GObj* victim = Player_GetEntity(0);
    HSD_GObj* attacker = Player_GetEntity(1);
    static unsigned pulse_until, next_attack, previous_frame;
    static int ready, initial_damage, observed, refused;
    static int keyboard_released, keyboard_config_failed;
    int keyboard_mode = getenv("MELEE_DIAGNOSTIC_P2_CONTACT_KEYBOARD") != NULL;
    memset(p1, 0, sizeof(*p1));
    if (frame < previous_frame) {
        pulse_until = next_attack = 0;
        ready = observed = refused = 0;
        keyboard_released = keyboard_config_failed = 0;
    }
    previous_frame = frame;
    if (victim && attacker && gm_GetCurrentGameMode() == GM_VS &&
        stage_info.grkind == Gr_Kind_Last &&
        Player_GetPlayerSlotType(0) == 0 && Player_GetPlayerSlotType(1) == 0 &&
        Player_GetPadPort(0) == 0 && Player_GetPadPort(1) == 1) {
        Fighter* a = attacker->user_data;
        Fighter* v = victim->user_data;
        float dx = v->cur_pos.x - a->cur_pos.x;
        float dy = v->cur_pos.y - a->cur_pos.y;
        float distance = dx < 0 ? -dx : dx;
        float height = dy < 0 ? -dy : dy;
        if (!ready) {
            initial_damage = Player_GetDamage(0);
            ready = 1;
            OSReport("NATIVE_P2_CONTACT_READY frame=%u victim_damage=%d ports=0,1 human_slots=1\n", frame, initial_damage);
        }
        if (!observed && Player_GetDamage(0) > initial_damage) {
            observed = 1;
            OSReport("NATIVE_P2_CONTACT_DAMAGE_OBSERVED frame=%u before=%d after=%d\n", frame, initial_damage, Player_GetDamage(0));
#if defined(_WIN32)
            {
                char* path = NULL;
                size_t size = 0;
                if (_dupenv_s(&path, &size, "MELEE_DIAGNOSTIC_P2_CONTACT_CAPTURE") == 0 && path && path[0])
                    OSReport("NATIVE_P2_CONTACT_CAPTURE armed=%d frame=%u\n", _putenv_s("AURORA_DIAGNOSTIC_COLOR_CAPTURE", path) == 0, frame);
                free(path);
            }
#endif
        }
        if (!observed && !keyboard_released && !keyboard_config_failed && frame >= 180) {
            if (frame < pulse_until) p2.button = PAD_BUTTON_A;
            else if (distance > 7.0F || a->facing_dir * dx < 0)
                p2.stickX = dx > 0 ? 25 : -25;
            else if (height < 3.0F && frame >= next_attack &&
                     ftLib_GetGroundAir(attacker) == GA_Ground &&
                     a->motion_id == ftCo_MS_Wait) {
                if (keyboard_mode) {
                    /* SDL scancode9 was observed for F in native Controls
                     * capture. Check its name before using that recorded key.
                     * Configure through the same session API as the menu, then
                     * stop staging P2 entirely: only ordinary PADRead can attack. */
                    PADKeyButtonBinding binding = { .scancode = 9, .padButton = PAD_BUTTON_A };
                    if (strcmp(PADGetKeyName(9), "F") == 0 &&
                        PADInstallKeyboardPreset(1) && PADSetKeyButtonBinding(1, binding)) {
                        PADClearVirtualStatus(1);
                        keyboard_released = 1;
                        OSReport("NATIVE_P2_CONTACT_KEYBOARD_READY frame=%u key=F dx=%f dy=%f facing=%f p1_damage=%d p2_pad_staging_disabled=1\n", frame, dx, dy, a->facing_dir, Player_GetDamage(0));
                    } else {
                        keyboard_config_failed = 1;
                        OSReport("NATIVE_P2_CONTACT_KEYBOARD_CONFIG_REFUSED frame=%u\n", frame);
                    }
                } else {
                    p2.button = PAD_BUTTON_A;
                    pulse_until = frame + 4;
                    next_attack = frame + 60;
                    OSReport("NATIVE_P2_CONTACT_ATTACK frame=%u dx=%f dy=%f facing=%f item=%d\n", frame, dx, dy, a->facing_dir, a->item_gobj != NULL);
                }
            }
        }
        if (frame % 30 == 0 || p2.button)
            OSReport("NATIVE_P2_CONTACT frame=%u p1_damage=%d p2_damage=%d p1_x=%f p1_y=%f p2_x=%f p2_y=%f motion=%d facing=%f request_x=%d request_button=%u held=%u pressed=%u observed=%d\n", frame, Player_GetDamage(0), Player_GetDamage(1), v->cur_pos.x, v->cur_pos.y, a->cur_pos.x, a->cur_pos.y, a->motion_id, a->facing_dir, p2.stickX, p2.button, a->input.held_buttons[0], a->input.pressed_buttons, observed);
    } else if (!refused) {
        refused = 1;
        OSReport("NATIVE_P2_CONTACT_REFUSED frame=%u requires_vs_final_destination_human_ports_0_1=1\n", frame);
    }
    if (!keyboard_released && !HSD_PadStageDiagnosticInput(1, &p2))
        OSReport("NATIVE_P2_CONTACT_INPUT_REFUSED frame=%u\n", frame);
}
#endif
