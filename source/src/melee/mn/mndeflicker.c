#include <Runtime/platform.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(TARGET_PC) && defined(MELEE_NO_SAVES)
#include <aurora/aurora.h>
#include "../../pc/renderer_restart.h"
#endif

#include <melee/it/forward.h>
#include <sysdolphin/baselib/forward.h>

#include "inlines.h"
#include "mnmain.h"
#include "types.h"
#include <melee/gm/gmmain_lib.h>
#include <melee/lb/lbarchive.h>
#include <melee/lb/lbspdisplay.h>
#include <melee/sc/types.h>
#include <sysdolphin/baselib/debug.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjgxlink.h>
#include <sysdolphin/baselib/gobjobject.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/gobjproc.h>
#include <sysdolphin/baselib/gobjuserdata.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/memory.h>
#include <sysdolphin/baselib/sislib.h>

void mnDeflicker_8024A168(HSD_GObj* gobj);
void mnDeflicker_8024A2E8(HSD_GObj* gobj);
void mnDeflicker_8024A344(HSD_GObj* gobj);
void mnDeflicker_8024A3E8(HSD_GObj* gobj);
void mnDeflicker_8024A4BC(HSD_GObj* arg0);
void mnDeflicker_8024A6C4(HSD_GObj* arg0);

void lbCardGame_SaveChanges(void);

#if defined(TARGET_PC) && defined(MELEE_NO_SAVES)
extern HSD_GObj* mnDeflicker_804D6C38;
/* PC's native Settings > Display page doubles as its in-game graphics menu.
 * The values below are session-only renderer preferences; none are written
 * to the memory-card backend. Keep this UI in SIS so it is drawn by the game
 * scene rather than Aurora's separate host-side ImGui dock. */
#define MN_PC_GRAPHICS_LINE_COUNT 24
static HSD_Text* mnDeflicker_pc_graphics_lines[MN_PC_GRAPHICS_LINE_COUNT];
static HSD_Text* mnDeflicker_pc_graphics_text;
static u8 mnDeflicker_pc_graphics_selection;
static u32 mnDeflicker_pc_drawn_pack;
static int mnDeflicker_pc_drawn_pack_pending;
static char mnDeflicker_pc_graphics_feedback[72] = "Ready";
static AuroraBackend mnDeflicker_pc_renderer_choices[4];
static u32 mnDeflicker_pc_renderer_choice_count;
static u32 mnDeflicker_pc_renderer_choice;
static int mnDeflicker_pc_renderer_restart_pending;

static const u32 mnDeflicker_pc_render_heights[] = {
    0, 240, 360, 480, 720, 1080, 1440, 2160, 4320,
};

static const char* mnDeflicker_pc_height_label(u32 height)
{
    switch (height) {
    case 0: return "Window";
    case 240: return "240p";
    case 360: return "360p";
    case 480: return "480p";
    case 720: return "720p";
    case 1080: return "1080p";
    case 1440: return "1440p";
    case 2160: return "4K";
    case 4320: return "8K";
    default: return "Unavailable";
    }
}

static const char* mnDeflicker_pc_renderer_label(AuroraBackend backend)
{
    switch (backend) {
    case BACKEND_AUTO: return "Auto";
    case BACKEND_D3D11: return "D3D11";
    case BACKEND_D3D12: return "D3D12";
    case BACKEND_VULKAN: return "Vulkan";
    default: return "Unsupported";
    }
}

static AuroraBackend mnDeflicker_pc_startup_renderer(void)
{
    const char* renderer = getenv("MELEE_STARTUP_RENDERER");
    if (renderer == NULL || !strcmp(renderer, "auto")) return BACKEND_AUTO;
    if (!strcmp(renderer, "d3d11")) return BACKEND_D3D11;
    if (!strcmp(renderer, "d3d12")) return BACKEND_D3D12;
    if (!strcmp(renderer, "vulkan")) return BACKEND_VULKAN;
    return BACKEND_AUTO;
}

/* Aurora reports its compiled/available backends. Device-specific startup is
 * then verified in a replacement process before the current game is closed. */
static void mnDeflicker_pc_renderer_choices_refresh(void)
{
    size_t available_count = 0;
    const AuroraBackend* available = aurora_get_available_backends(&available_count);
    size_t i;
    u32 j;

    mnDeflicker_pc_renderer_choices[0] = BACKEND_AUTO;
    mnDeflicker_pc_renderer_choice_count = 1;
    for (i = 0; i < available_count; ++i) {
        AuroraBackend backend = available[i];
        if (backend != BACKEND_D3D11 && backend != BACKEND_D3D12 &&
            backend != BACKEND_VULKAN)
            continue;
        for (j = 1; j < mnDeflicker_pc_renderer_choice_count; ++j)
            if (mnDeflicker_pc_renderer_choices[j] == backend) break;
        if (j == mnDeflicker_pc_renderer_choice_count &&
            mnDeflicker_pc_renderer_choice_count <
                ARRAY_SIZE(mnDeflicker_pc_renderer_choices))
            mnDeflicker_pc_renderer_choices[
                mnDeflicker_pc_renderer_choice_count++] = backend;
    }
    if (mnDeflicker_pc_renderer_choice >=
        mnDeflicker_pc_renderer_choice_count)
        mnDeflicker_pc_renderer_choice = 0;
}

static void mnDeflicker_pc_graphics_draw(void)
{
    Menu* menu = GET_MENU(mnDeflicker_804D6C38);
    static const char* aspect_labels[] = { "4:3", "16:9", "16:10", "21:9" };
    static const u32 texture_values[] = { 100, 50, 25 };
    static const u32 shadow_values[] = { 64, 128, 256 };
    static const char* enabled[] = { "Off", "On" };
    static const GXColor normal_color = { 0xE8, 0xE2, 0xD2, 0xFF };
    /* Match the warm gold used by the existing name/menu SIS palette. */
    static const GXColor selected_color = { 0xA6, 0x81, 0x3D, 0xFF };
    static const GXColor help_color = { 0xA9, 0xB5, 0xC5, 0xFF };
    /* Cover the bright authored sample preview with a native SIS panel while
     * keeping the surrounding Screen Display artwork visible. */
    static const GXColor panel_color = { 0x05, 0x0A, 0x17, 0xFC };
    u32 height;
    u32 i;
    char labels[9][32];
    char values[9][32];
    char row_label[40];
    char feedback[72];

    if (mnDeflicker_pc_graphics_text == NULL) return;
    height = aurora_get_render_height();
    snprintf(labels[0], sizeof(labels[0]), "Aspect ratio");
    snprintf(values[0], sizeof(values[0]), "%s",
             aspect_labels[aurora_get_display_preset() & 3]);
    snprintf(labels[1], sizeof(labels[1]), "Render height");
    snprintf(values[1], sizeof(values[1]), "%s",
             mnDeflicker_pc_height_label(height));
    snprintf(labels[2], sizeof(labels[2]), "Shadows");
    snprintf(values[2], sizeof(values[2]), "%s",
             enabled[aurora_get_fighter_shadows() != 0]);
    for (i = 0; i < 3 && shadow_values[i] != aurora_get_fighter_shadow_resolution(); ++i) {}
    snprintf(labels[3], sizeof(labels[3]), "Shadow resolution");
    snprintf(values[3], sizeof(values[3]), "%s",
             i < 3 ? (i == 0 ? "64 Low" : i == 1 ? "128 Medium" : "256 High") : "Unknown");
    for (i = 0; i < 3 && texture_values[i] != aurora_get_texture_quality(); ++i) {}
    snprintf(labels[4], sizeof(labels[4]), "Texture resolution");
    snprintf(values[4], sizeof(values[4]), "%s",
             i < 3 ? (i == 0 ? "100 percent" : i == 1 ? "50 percent" : "25 percent") : "Unknown");
    snprintf(labels[5], sizeof(labels[5]), "Lighting");
    snprintf(values[5], sizeof(values[5]), "%s",
             enabled[aurora_get_lighting_enabled() != 0]);
    snprintf(labels[6], sizeof(labels[6]), "Low detail models");
    snprintf(values[6], sizeof(values[6]), "%s",
             enabled[aurora_get_low_poly_fighters() != 0]);
    mnDeflicker_pc_renderer_choices_refresh();
    snprintf(labels[7], sizeof(labels[7]), "Renderer");
    snprintf(values[7], sizeof(values[7]), "%s",
             mnDeflicker_pc_renderer_label(
                 mnDeflicker_pc_renderer_choices[mnDeflicker_pc_renderer_choice]));
    mnDeflicker_pc_drawn_pack = aurora_get_texture_pack();
    mnDeflicker_pc_drawn_pack_pending = aurora_is_texture_pack_pending();
    snprintf(labels[8], sizeof(labels[8]), "Texture pack");
    snprintf(values[8], sizeof(values[8]), "%s",
             mnDeflicker_pc_drawn_pack_pending ? "Applying..." :
             mnDeflicker_pc_drawn_pack ? "HD" :
             aurora_is_hd_texture_pack_available() ? "Original" : "Original only");
    snprintf(feedback, sizeof(feedback), "%s",
             mnDeflicker_pc_graphics_feedback);

    /* Keep the native SIS font and frame, but place the list in the clear right
     * side of the Screen Display card so it does not collide with its authored
     * Deflicker tab. A gold selected row and caret mirror the menu's existing
     * focus treatment; the hint lines follow the native navigation model. */
    for (i = 0; i < MN_PC_GRAPHICS_LINE_COUNT; ++i) {
        HSD_Text* line = mnDeflicker_pc_graphics_lines[i];
        const char* content;
        u32 row = i >= 3 ? (i - 3) / 2 : 0;
        u32 column = i >= 3 ? (i - 3) % 2 : 0;
        GXColor color;

        if (line != NULL) {
            HSD_SisLib_803A5CC4(line);
        }
        line = HSD_SisLib_803A6754(0, mn_804D6BB4);
        mnDeflicker_pc_graphics_lines[i] = line;
        line->pos_z = 17.0F;
        line->box_size_x = 188.0F;
        line->box_size_y = 26.0F;
        line->default_kerning = 1;
        line->default_alignment = 0;

        if (i == 0) {
            content = "";
            line->pos_x = -3.25F;
            line->pos_y = -8.2F;
            line->font_size.x = line->font_size.y = 0.030F;
            line->box_size_x = 850.0F;
            line->box_size_y = 560.0F;
            line->bg_color = panel_color;
            color = panel_color;
        } else if (i == 1) {
            content = "Graphics Settings";
            line->pos_x = -2.8F;
            line->pos_y = -5.55F;
            line->font_size.x = line->font_size.y = 0.030F;
            color = selected_color;
        } else if (i == 2) {
            content = "Display and performance";
            line->pos_x = -2.8F;
            line->pos_y = -4.55F;
            line->font_size.x = line->font_size.y = 0.021F;
            color = help_color;
        } else if (i < 21) {
            f32 y = -3.45F + (0.84F * (f32) row);
            line->pos_y = y;
            line->font_size.x = line->font_size.y = 0.023F;
            color = row == mnDeflicker_pc_graphics_selection
                        ? selected_color
                        : normal_color;
            if (row == 8 && !aurora_is_hd_texture_pack_available())
                color = help_color;
            if (column == 0) {
                snprintf(row_label, sizeof(row_label), "%s%s",
                         row == mnDeflicker_pc_graphics_selection ? "> " : "  ",
                         labels[row]);
                content = row_label;
                line->pos_x = -3.15F;
            } else {
                content = values[row];
                line->pos_x = 6.7F;
            }
        } else if (i == 21) {
            content = "Up and down select    Left and right adjust";
            line->pos_x = -2.8F;
            line->pos_y = 4.3F;
            line->font_size.x = line->font_size.y = 0.019F;
            color = help_color;
        } else if (i == 22) {
            content = "A restarts renderer    B returns";
            line->pos_x = -2.8F;
            line->pos_y = 5.1F;
            line->font_size.x = line->font_size.y = 0.019F;
            color = help_color;
        } else {
            content = feedback;
            line->pos_x = -2.8F;
            line->pos_y = 5.9F;
            line->font_size.x = line->font_size.y = 0.019F;
            color = selected_color;
        }
        HSD_SisLib_803A6B98(line, 0.0F, 0.0F, "%s", content);
        HSD_SisLib_803A74F0(line, 0, &color);
    }
    menu->text = mnDeflicker_pc_graphics_lines[0];
    mnDeflicker_pc_graphics_text = menu->text;
    if (getenv("MELEE_DIAGNOSTIC_GRAPHICS_MENU_TRACE")) {
        OSReport("NATIVE_PC_GRAPHICS_MENU selection=%u aspect=%u render_height=%u shadows=%u shadow_size=%u texture_quality=%u lighting=%u low_poly=%u renderer_choice=%u renderer=%s active=%s texture_pack=%u pack_available=%d pack_pending=%d\n",
                 mnDeflicker_pc_graphics_selection,
                 aurora_get_display_preset(), aurora_get_render_height(),
                 aurora_get_fighter_shadows(),
                 aurora_get_fighter_shadow_resolution(),
                 aurora_get_texture_quality(), aurora_get_lighting_enabled(),
                 aurora_get_low_poly_fighters(),
                 mnDeflicker_pc_renderer_choice,
                 mnDeflicker_pc_renderer_label(
                     mnDeflicker_pc_renderer_choices[
                         mnDeflicker_pc_renderer_choice]),
                 mnDeflicker_pc_renderer_label(aurora_get_backend()),
                 aurora_get_texture_pack(), aurora_is_hd_texture_pack_available(),
                 aurora_is_texture_pack_pending());
    }
}

static int mnDeflicker_pc_cycle(u32 current, const u32* values, u32 count,
                                int direction, u32* next)
{
    u32 i;
    for (i = 0; i < count && values[i] != current; ++i) {}
    if (i == count) return 0;
    i = (u32) (((int) i + direction + (int) count) % (int) count);
    *next = values[i];
    return 1;
}

static void mnDeflicker_pc_graphics_adjust(int direction)
{
    static const u32 texture_values[] = { 100, 50, 25 };
    static const u32 shadow_values[] = { 64, 128, 256 };
    u32 next;
    int applied = 0;
    mnDeflicker_pc_graphics_feedback[0] = '\0';
    switch (mnDeflicker_pc_graphics_selection) {
    case 0:
        applied = aurora_set_display_preset(
            (aurora_get_display_preset() + 4 + direction) % 4);
        break;
    case 1: {
        u32 current = aurora_get_render_height();
        if (mnDeflicker_pc_cycle(current, mnDeflicker_pc_render_heights,
                                 ARRAY_SIZE(mnDeflicker_pc_render_heights),
                                 direction, &next)) {
            applied = aurora_set_render_height(next);
        }
        if (!applied) {
            snprintf(mnDeflicker_pc_graphics_feedback,
                     sizeof(mnDeflicker_pc_graphics_feedback),
                     "That render size is unavailable on this device");
            return;
        }
        break;
    }
    case 2:
        applied = aurora_set_fighter_shadows(!aurora_get_fighter_shadows());
        break;
    case 3:
        if (mnDeflicker_pc_cycle(aurora_get_fighter_shadow_resolution(),
                                 shadow_values, ARRAY_SIZE(shadow_values),
                                 direction, &next))
            applied = aurora_set_fighter_shadow_resolution(next);
        break;
    case 4:
        if (mnDeflicker_pc_cycle(aurora_get_texture_quality(), texture_values,
                                 ARRAY_SIZE(texture_values), direction, &next))
            applied = aurora_set_texture_quality(next);
        break;
    case 5:
        applied = aurora_set_lighting_enabled(!aurora_get_lighting_enabled());
        break;
    case 6:
        applied = aurora_set_low_poly_fighters(!aurora_get_low_poly_fighters());
        break;
    case 7: {
        u32 i;
        if (!mnDeflicker_pc_renderer_choice_count) break;
        i = (u32) (((int) mnDeflicker_pc_renderer_choice + direction +
                    (int) mnDeflicker_pc_renderer_choice_count) %
                   (int) mnDeflicker_pc_renderer_choice_count);
        mnDeflicker_pc_renderer_choice = i;
        applied = 1;
        snprintf(mnDeflicker_pc_graphics_feedback,
                 sizeof(mnDeflicker_pc_graphics_feedback),
                 "Renderer staged; press A to restart and apply");
        break;
    }
    case 8:
        if (!aurora_is_hd_texture_pack_available()) {
            snprintf(mnDeflicker_pc_graphics_feedback,
                     sizeof(mnDeflicker_pc_graphics_feedback),
                     "Install HD textures to enable this option");
            if (getenv("MELEE_DIAGNOSTIC_GRAPHICS_MENU_TRACE"))
                OSReport("NATIVE_PC_GRAPHICS_PACK unavailable=1\n");
            return;
        }
        if (!aurora_is_texture_pack_pending())
            applied = aurora_set_texture_pack(!aurora_get_texture_pack());
        snprintf(mnDeflicker_pc_graphics_feedback,
                 sizeof(mnDeflicker_pc_graphics_feedback),
                 "Applying texture choice...");
        break;
    }
    if ((mnDeflicker_pc_graphics_selection != 7 &&
         mnDeflicker_pc_graphics_selection != 8) || !applied)
        snprintf(mnDeflicker_pc_graphics_feedback,
                 sizeof(mnDeflicker_pc_graphics_feedback), "%s",
                 applied ? "Setting changed" : "Setting could not be changed");
    if (getenv("MELEE_DIAGNOSTIC_GRAPHICS_MENU_TRACE")) {
        OSReport("NATIVE_PC_GRAPHICS_ADJUST selection=%u applied=%d direction=%d aspect=%u render_height=%u shadows=%u shadow_size=%u texture_quality=%u lighting=%u low_poly=%u\n",
                 mnDeflicker_pc_graphics_selection, applied, direction,
                 aurora_get_display_preset(), aurora_get_render_height(),
                 aurora_get_fighter_shadows(),
                 aurora_get_fighter_shadow_resolution(),
                 aurora_get_texture_quality(), aurora_get_lighting_enabled(),
                 aurora_get_low_poly_fighters());
    }
    if (applied) sfxMove();
}

static void mnDeflicker_pc_graphics_apply_renderer(void)
{
    const AuroraBackend backend = mnDeflicker_pc_renderer_choices[
        mnDeflicker_pc_renderer_choice];
    const char* name;
    if (backend == mnDeflicker_pc_startup_renderer()) {
        snprintf(mnDeflicker_pc_graphics_feedback,
                 sizeof(mnDeflicker_pc_graphics_feedback),
                 "This renderer is already active");
        return;
    }
    switch (backend) {
    case BACKEND_AUTO: name = "auto"; break;
    case BACKEND_D3D11: name = "d3d11"; break;
    case BACKEND_D3D12: name = "d3d12"; break;
    case BACKEND_VULKAN: name = "vulkan"; break;
    default:
        snprintf(mnDeflicker_pc_graphics_feedback,
                 sizeof(mnDeflicker_pc_graphics_feedback),
                 "That renderer is not supported by this build");
        return;
    }
    if (!MeleePc_RendererRestartStart(name)) {
        snprintf(mnDeflicker_pc_graphics_feedback,
                 sizeof(mnDeflicker_pc_graphics_feedback),
                 "Could not start renderer switch; current game is unchanged");
        return;
    }
    mnDeflicker_pc_renderer_restart_pending = 1;
    snprintf(mnDeflicker_pc_graphics_feedback,
             sizeof(mnDeflicker_pc_graphics_feedback),
             "Starting renderer; current game will close only when ready");
}

static void mnDeflicker_pc_graphics_destroy(void)
{
    u32 i;
    /* The page transition owns/frees line zero through menu->text. */
    for (i = 1; i < MN_PC_GRAPHICS_LINE_COUNT; ++i) {
        if (mnDeflicker_pc_graphics_lines[i] != NULL) {
            HSD_SisLib_803A5CC4(mnDeflicker_pc_graphics_lines[i]);
            mnDeflicker_pc_graphics_lines[i] = NULL;
        }
    }
    mnDeflicker_pc_graphics_lines[0] = NULL;
    mnDeflicker_pc_graphics_text = NULL;
}
#endif

unsigned char mnDeflicker_804D6C3C;
HSD_GObj* mnDeflicker_804D6C38;
StaticModelDesc mnDeflicker_804A08B8;

AnimLoopSettings mnDeflicker_803EEFD8 = { 0.0F, 19.0F, -0.1F };
AnimLoopSettings mnDeflicker_803EEFE4 = { 20.0F, 30.0F, -0.1F };
AnimLoopSettings mnDeflicker_803EEFF0 = { 50.0F, 350.0F, 50.0F };

void mnDeflicker_8024A168(HSD_GObj* gobj)
{
    u64 events;
    HSD_JObj* asd[2];
    int stack[5];
    Menu* menu = GET_MENU(mnDeflicker_804D6C38);
    if (mn_804D6BC8.cooldown != 0) {
        Menu_DecrementAnimTimer();
        return;
    }
#if defined(TARGET_PC) && defined(MELEE_NO_SAVES)
    if (mnDeflicker_pc_graphics_text != NULL) {
        if (mnDeflicker_pc_drawn_pack != aurora_get_texture_pack() ||
            mnDeflicker_pc_drawn_pack_pending != aurora_is_texture_pack_pending()) {
            snprintf(mnDeflicker_pc_graphics_feedback,
                     sizeof(mnDeflicker_pc_graphics_feedback), "%s",
                     aurora_get_texture_pack() ? "HD textures enabled" :
                     aurora_is_hd_texture_pack_available() ? "Original textures restored" :
                     "HD textures could not be loaded");
            mnDeflicker_pc_graphics_draw();
        }
        events = Menu_GetAllInputs();
        if (mnDeflicker_pc_renderer_restart_pending) {
            MeleePc_RendererRestartPoll status =
                MeleePc_RendererRestartPollChild();
            if (status == MELEE_PC_RENDERER_RESTART_READY) {
                mnDeflicker_pc_renderer_restart_pending = 0;
                OSReport("NATIVE_RENDERER_RESTART parent=ready replacement_backend_verified=1\n");
                aurora_handle_exit();
                return;
            } else if (status == MELEE_PC_RENDERER_RESTART_FAILED) {
                mnDeflicker_pc_renderer_restart_pending = 0;
                snprintf(mnDeflicker_pc_graphics_feedback,
                         sizeof(mnDeflicker_pc_graphics_feedback),
                         "Renderer failed to start; current game is unchanged");
                mnDeflicker_pc_graphics_draw();
            } else {
                return;
            }
        }
        if (events & MenuInput_Back) {
            sfxBack();
            mn_804A04F0.entering_menu = 0;
            mnDeflicker_pc_graphics_destroy();
            mn_80229894(4, SEL_SETTINGS_DISPLAY, 3);
            return;
        } else if (events & MenuInput_Up) {
            mnDeflicker_pc_graphics_selection =
                (mnDeflicker_pc_graphics_selection + 8) % 9;
            sfxMove();
            mnDeflicker_pc_graphics_draw();
        } else if (events & MenuInput_Down) {
            mnDeflicker_pc_graphics_selection =
                (mnDeflicker_pc_graphics_selection + 1) % 9;
            sfxMove();
            mnDeflicker_pc_graphics_draw();
        } else if (events & (MenuInput_Left | MenuInput_Right |
                             MenuInput_Confirm | MenuInput_AButton)) {
            int direction = (events & MenuInput_Left) ? -1 : 1;
            if (mnDeflicker_pc_graphics_selection == 7 &&
                (events & (MenuInput_Confirm | MenuInput_AButton)))
                mnDeflicker_pc_graphics_apply_renderer();
            else
                mnDeflicker_pc_graphics_adjust(direction);
            mnDeflicker_pc_graphics_draw();
        }
        return;
    }
#endif
    events = Menu_GetAllInputs();
    if ((events & MenuInput_Back)) {
        sfxBack();
        gmMainLib_8015F4F4(
            ((Menu*) mnDeflicker_804D6C38->user_data)->cursor); // inline
        lbCardGame_SaveChanges();
        mn_804A04F0.entering_menu = 0;
        mn_80229894(4, 2, 3);
    } else if (mnDeflicker_804D6C3C && (events & MenuInput_AButton)) {
        u32 x;
        PAD_STACK(4);
        sfxMove();
        menu->cursor = (menu->cursor == 0);
        x = menu->cursor; // inline arg?
        lb_80011E24(mnDeflicker_804D6C38->hsd_obj, asd, 5, -1);
        HSD_JObjReqAnimAll(asd[0], (f32) x);
        mn_8022F3D8(asd[0], 0xFF, 0x20);
        HSD_JObjAnimAll(asd[0]);
        gmMainLib_8015F588(x);
        gmMainLib_8015F4F4(
            ((Menu*) mnDeflicker_804D6C38->user_data)->cursor); // inline
    }
}

void mnDeflicker_8024A2E8(HSD_GObj* gobj)
{
    int x[2];
    float f = mn_8022EC18(GET_JOBJ(gobj), &mnDeflicker_803EEFE4, 0x400);
    if (f == mnDeflicker_803EEFE4.end_frame) {
        HSD_GObjFree(gobj);
    }
}

void mnDeflicker_8024A344(HSD_GObj* gobj)
{
    Menu* menu = GET_MENU(gobj);
    HSD_JObj* jobj = GET_JOBJ(gobj);
    if (mn_804A04F0.cur_menu != 0x15) {
        HSD_GObjProc* p;
        HSD_GObjProc_RemoveProc(HSD_GObj_CurrentInvokedProc);
        p = HSD_GObj_SetupProc(gobj, &mnDeflicker_8024A2E8, 0);
        p->flags_3 = HSD_GObj_804D783C;
        HSD_SisLib_803A5CC4(menu->text);
    } else {
        HSD_JObj* asd;
        lb_80011E24(jobj, &asd, 6, -1);
        mn_8022ED6C(asd, &mnDeflicker_803EEFF0);
    }
}

void mnDeflicker_8024A3E8(HSD_GObj* gobj)
{
    Menu* menu = GET_MENU(gobj);
    HSD_JObj* jobj = GET_JOBJ(gobj);
    HSD_GObjProc* p;
    if (mn_804A04F0.cur_menu != 0x15) {
        HSD_GObjProc_RemoveProc(HSD_GObj_CurrentInvokedProc);
        p = HSD_GObj_SetupProc(gobj, &mnDeflicker_8024A2E8, 0);
        p->flags_3 = HSD_GObj_804D783C;
        HSD_SisLib_803A5CC4(menu->text);
    } else {
        float f = mn_8022EC18(jobj, &mnDeflicker_803EEFD8, 0x400);
        if (f == mnDeflicker_803EEFD8.end_frame) {
            HSD_GObjProc_RemoveProc(HSD_GObj_CurrentInvokedProc);
            p = HSD_GObj_SetupProc(gobj, &mnDeflicker_8024A344, 0);
            p->flags_3 = HSD_GObj_804D783C;
            mnDeflicker_804D6C3C = 1;
        }
    }
}

static void mnDeflicker_8024A4BC_inline(void)
{
    Menu* menu = GET_MENU(mnDeflicker_804D6C38);
    if (menu->text) {
        HSD_SisLib_803A5CC4(menu->text);
    }
#if defined(TARGET_PC) && defined(MELEE_NO_SAVES)
    mnDeflicker_pc_graphics_text = (HSD_Text*) 1;
    mnDeflicker_pc_graphics_selection = 0;
    mnDeflicker_pc_renderer_choice_count = 0;
    mnDeflicker_pc_renderer_choices_refresh();
    {
        AuroraBackend startup_backend = mnDeflicker_pc_startup_renderer();
        u32 i;
        for (i = 0; i < mnDeflicker_pc_renderer_choice_count; ++i) {
            if (mnDeflicker_pc_renderer_choices[i] == startup_backend) {
                mnDeflicker_pc_renderer_choice = i;
                break;
            }
        }
    }
    snprintf(mnDeflicker_pc_graphics_feedback,
             sizeof(mnDeflicker_pc_graphics_feedback),
             "Left or right changes this setting");
    mnDeflicker_pc_graphics_draw();
#else
    Menu_InitCenterText(menu, 0xBD);
#endif
}

void mnDeflicker_8024A4BC(HSD_GObj* arg0)
{
    int fake1[3];
    HSD_JObj* sp1C;
    HSD_GObj* gobj;
    HSD_GObjProc* proc;
    HSD_JObj* jobj;
    u8 temp_r29;
    Menu* user_data;
    PAD_STACK(4);

    gobj = GObj_Create(HSD_GOBJ_CLASS_ITEM, 7U, 0x80);
    mnDeflicker_804D6C38 = gobj;
    jobj = HSD_JObjLoadJoint(mnDeflicker_804A08B8.joint);
    temp_r29 = HSD_GObj_JObjKind; // ty permuter, why does this fix everything?
    HSD_GObjObject_80390A70(gobj, temp_r29, jobj);
    GObj_SetupGXLink(gobj, HSD_GObj_JObjCallback, 4, 0x80);
    HSD_JObjAddAnimAll(jobj, mnDeflicker_804A08B8.animjoint,
                       mnDeflicker_804A08B8.matanim_joint,
                       mnDeflicker_804A08B8.shapeanim_joint);
    HSD_JObjReqAnimAll(jobj, 0.0F);
    HSD_JObjAnimAll(jobj);
    user_data = HSD_MemAlloc(sizeof(*user_data));
    HSD_ASSERTREPORT(344, user_data, "Can't get user_data.\n");
    user_data->cursor = gmMainLib_8015F4E8();
    user_data->text = NULL;
    GObj_InitUserData(gobj, 0, HSD_Free, user_data);
    proc = HSD_GObj_SetupProc(gobj, mnDeflicker_8024A3E8, 0U);
    proc->flags_3 = HSD_GObj_804D783C;
    temp_r29 = user_data->cursor;
    lb_80011E24(GET_JOBJ(gobj), &sp1C, 5, -1);
    HSD_JObjReqAnimAll(sp1C, (f32) temp_r29);
    mn_8022F3D8(sp1C, 0xFF, 0x20);
    HSD_JObjAnimAll(sp1C);
    gmMainLib_8015F588(temp_r29);
    HSD_JObjReqAnimAll(jobj, 0.0F);
    mn_8022F3D8(jobj, 0xFF, 0x400);
    HSD_JObjAnimAll(jobj);
    mnDeflicker_8024A4BC_inline();
}

void mnDeflicker_8024A6C4(HSD_GObj* arg0)
{
    HSD_GObjProc* temp_r3;
    HSD_Archive* archive;

    mn_804D6BC8.cooldown = 5;
    mn_804A04F0.prev_menu = mn_804A04F0.cur_menu;
    mn_804A04F0.cur_menu = 0x15;
    mn_804A04F0.hovered_selection = 0;
    mnDeflicker_804D6C3C = 0;
    archive = mn_804D6BB8;
    lbArchive_LoadSections(
        archive, &mnDeflicker_804A08B8.joint, "MenMainConDf_Top_joint",
        &mnDeflicker_804A08B8.animjoint, "MenMainConDf_Top_animjoint",
        &mnDeflicker_804A08B8.matanim_joint, "MenMainConDf_Top_matanim_joint",
        &mnDeflicker_804A08B8.shapeanim_joint,
        "MenMainConDf_Top_shapeanim_joint", 0);
    mnDeflicker_8024A4BC(arg0);
    temp_r3 =
        HSD_GObj_SetupProc(GObj_Create(0, 1, 0x80), mnDeflicker_8024A168, 0);
    temp_r3->flags_3 = HSD_GObj_804D783C;
}
