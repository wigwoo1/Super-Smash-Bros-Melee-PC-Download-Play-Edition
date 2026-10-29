#include "mnsound.h"

#if defined(TARGET_PC) && defined(MELEE_NO_SAVES)
#include <aurora/audio.h>
#include <stdio.h>
#include <stdlib.h>
#endif

#include <melee/it/forward.h>

#include "inlines.h"
#include "mnmain.h"
#include "types.h"
#include <melee/gm/gm_1601.h>
#include <melee/gm/gmmain_lib.h>
#include <melee/lb/lbarchive.h>
#include <melee/lb/lbcardgame.h>
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

/* 3EEED8 */ static AnimLoopSettings mnSound_803EEED8[] = {
    { 0.0F, 199.0F, 0.0F },
    { 0.0F, 29.0F, 0.0F },
    { 30.0F, 59.0F, 30.0F },
    { 10.0F, 15.0F, -0.1F },
    { 0.0F, 5.0F, -0.1F }
};
static AnimLoopSettings volume_left = { 0.0F, 3.0F, -0.1F };
static AnimLoopSettings volume_right = { 4.0F, 7.0F, -0.1F };
/* 4A08A8 */ static StaticModelDesc mnSound_804A08A8;
/* 4D6C30 */ static HSD_GObj* mnSound_804D6C30;

#if defined(TARGET_PC) && defined(MELEE_NO_SAVES)
#define MN_PC_SOUND_LINES 10
static HSD_Text* mnSound_pc_lines[MN_PC_SOUND_LINES];
static int mnSound_pc_status = -1;
static u32 mnSound_pc_effective;

static void mnSound_pc_draw(void)
{
    Menu* menu = GET_MENU(mnSound_804D6C30);
    static const GXColor normal = { 0xE8, 0xE2, 0xD2, 0xFF };
    static const GXColor selected = { 0xA6, 0x81, 0x3D, 0xFF };
    static const GXColor help = { 0xA9, 0xB5, 0xC5, 0xFF };
    static const GXColor panel = { 0x05, 0x0A, 0x17, 0xFC };
    char mix[32], active[48];
    const char* status;
    u32 i;
    mnSound_pc_status = aurora_ax_output_status();
    mnSound_pc_effective = aurora_ax_output_channels();
    snprintf(mix, sizeof(mix), "%d", (s8) menu->unk3);
    snprintf(active, sizeof(active), "Active output: %s",
             mnSound_pc_effective == 6 ?
                 (aurora_ax_stereo_upmix_enabled() ? "5.1 upmix" : "5.1 output") : "Stereo");
    switch (mnSound_pc_status) {
    case AURORA_AUDIO_OUTPUT_PENDING: status = "Applying output; please wait"; break;
    case AURORA_AUDIO_OUTPUT_UNAVAILABLE: status = "5.1 unavailable. Using Stereo"; break;
    case AURORA_AUDIO_OUTPUT_ERROR: status = "Output change failed; check active output"; break;
    default: status = aurora_ax_stereo_upmix_enabled() && mnSound_pc_effective == 6 ?
        "Stereo expanded to extra speakers" : "Changes last for this session"; break;
    }
    /* Original center text is replaced by the game-owned SIS panel. Line zero
     * remains menu->text, so the page transition has one clear owner. */
    if (mnSound_pc_lines[0] == NULL && menu->text != NULL)
        HSD_SisLib_803A5CC4(menu->text);
    for (i = 0; i < MN_PC_SOUND_LINES; ++i) {
        HSD_Text* line;
        GXColor color = normal;
        const char* content;
        if (mnSound_pc_lines[i] != NULL)
            HSD_SisLib_803A5CC4(mnSound_pc_lines[i]);
        line = HSD_SisLib_803A6754(0, mn_804D6BB4);
        mnSound_pc_lines[i] = line;
        line->pos_z = 17.0F;
        line->pos_x = -13.5F;
        line->box_size_x = 800.0F;
        line->box_size_y = 28.0F;
        line->default_kerning = 1;
        line->default_alignment = 0;
        line->font_size.x = line->font_size.y = 0.025F;
        switch (i) {
        case 0:
            content = ""; line->pos_x = -14.5F; line->pos_y = -8.2F;
            line->font_size.x = line->font_size.y = 0.030F;
            line->box_size_x = 900.0F; line->box_size_y = 560.0F;
            line->bg_color = panel; color = panel; break;
        case 1:
            content = "Sound Settings"; line->pos_y = -6.0F;
            line->font_size.x = line->font_size.y = 0.033F;
            color = selected; break;
        case 2:
            content = "Audio output and music balance"; line->pos_y = -4.6F;
            line->font_size.x = line->font_size.y = 0.021F; color = help; break;
        case 3:
            content = menu->unk2 == 0 ? "> Output" : "  Output";
            line->pos_y = -2.5F; color = menu->unk2 == 0 ? selected : normal; break;
        case 4:
            content = aurora_ax_requested_output_channels() == 6 ?
                (aurora_ax_stereo_upmix_enabled() ? "5.1 upmix" : "5.1 output") : "Stereo";
            line->pos_x = 1.5F; line->pos_y = -2.5F;
            color = menu->unk2 == 0 ? selected : normal; break;
        case 5:
            content = menu->unk2 == 1 ? "> Music / effects" : "  Music / effects";
            line->pos_y = 0.0F; color = menu->unk2 == 1 ? selected : normal; break;
        case 6:
            content = mix; line->pos_x = 1.5F; line->pos_y = 0.0F;
            color = menu->unk2 == 1 ? selected : normal; break;
        case 7:
            content = active; line->pos_y = 1.8F; color = help; break;
        case 8:
            content = status; line->pos_y = 3.1F;
            line->font_size.x = line->font_size.y = 0.021F; color = help; break;
        default:
            content = "Up / down select   Left / right adjust   B returns";
            line->pos_y = 5.4F;
            line->font_size.x = line->font_size.y = 0.018F; color = help; break;
        }
        HSD_SisLib_803A6B98(line, 0.0F, 0.0F, "%s", content);
        HSD_SisLib_803A74F0(line, 0, &color);
    }
    menu->text = mnSound_pc_lines[0];
    if (getenv("MELEE_DIAGNOSTIC_AUDIO_MENU"))
        OSReport("NATIVE_SOUND_MENU selection=%u requested=%u effective=%u status=%d mix=%d upmix=%d\n",
                 menu->unk2, aurora_ax_requested_output_channels(), mnSound_pc_effective,
                 mnSound_pc_status, (s8) menu->unk3, aurora_ax_stereo_upmix_enabled());
}

static void mnSound_pc_destroy(void)
{
    u32 i;
    for (i = 1; i < MN_PC_SOUND_LINES; ++i) {
        if (mnSound_pc_lines[i] != NULL)
            HSD_SisLib_803A5CC4(mnSound_pc_lines[i]);
        mnSound_pc_lines[i] = NULL;
    }
    mnSound_pc_lines[0] = NULL;
    mnSound_pc_status = -1;
}
#endif

static f32 getVolumePosition(u8 sound_music_mix, const Vec3* pos_0,
                             const Vec3* pos_1)
{
    return (((f32) ((s8) sound_music_mix + 100) / 200.0F) *
            (pos_1->x - pos_0->x)) +
           pos_0->x;
}

static inline void updateVolumeAnim(HSD_JObj* jobj, u8 sound_music_mix,
                                    int direction, Vec3* pos_0, Vec3* pos_1,
                                    HSD_JObj** jobj_anim_0,
                                    HSD_JObj** jobj_anim_1,
                                    HSD_JObj** jobj_anim_2)
{
    lb_80011E24(jobj, jobj_anim_0, 6, -1);
    lb_80011E24(jobj, jobj_anim_1, 3, -1);
    lb_80011E24(jobj, jobj_anim_2, 4, -1);
    HSD_JObjGetTranslation(*jobj_anim_1, pos_0);
    HSD_JObjGetTranslation(*jobj_anim_2, pos_1);
    HSD_JObjSetTranslateX(*jobj_anim_0,
                          getVolumePosition(sound_music_mix, pos_0, pos_1));
    if (direction != 0) {
        HSD_JObjReqAnimAll(*jobj_anim_0, direction < 0
                                             ? volume_left.start_frame
                                             : volume_right.start_frame);
    }
    gm_801602C0(sound_music_mix);
}

static inline void updateChannelAnim(HSD_JObj* jobj, s32 channel,
                                     HSD_JObj** jobj_anim_0,
                                     HSD_JObj** jobj_anim_1,
                                     HSD_JObj** jobj_anim_2)
{
    f32 right_frame, left_frame;
    lb_80011E24(jobj, jobj_anim_0, 8, -1);
    lb_80011E24(jobj, jobj_anim_1, 10, -1);
    lb_80011E24(jobj, jobj_anim_2, 9, -1);
    left_frame = mn_8022F298(*jobj_anim_1);
    right_frame = mn_8022F298(*jobj_anim_2);

    HSD_JObjReqAnimAll(*jobj_anim_0, channel);
    HSD_JObjAnimAll(*jobj_anim_0);

    HSD_JObjReqAnimAll(*jobj_anim_1, left_frame);
    mn_8022F3D8(*jobj_anim_1, 0xFFU, MOBJ_MASK);
    HSD_JObjAnimAll(*jobj_anim_1);

    HSD_JObjReqAnimAll(*jobj_anim_2, right_frame);
    mn_8022F3D8(*jobj_anim_2, 0xFFU, MOBJ_MASK);
    HSD_JObjAnimAll(*jobj_anim_2);
}

static inline void updateCenterText(void)
{
    Menu* menu = mnSound_804D6C30->user_data;
    s32 text_id;
    if (menu->text != NULL) {
        HSD_SisLib_803A5CC4(menu->text);
    }
    if (menu->unk2 == 0) {
        text_id = 0xBB;
    } else {
        text_id = 0xBC;
    }
    Menu_InitCenterText(menu, text_id);
}

void mnSound_802492CC(HSD_GObj* gobj)
{
    u64 events;
    UNUSED u8 pad[12];
    u8 mix;
    Menu* menu = mnSound_804D6C30->user_data;
    if (mn_804D6BC8.cooldown != 0) {
        Menu_DecrementAnimTimer();
        return;
    }
    events = Menu_GetAllInputs();
#if defined(TARGET_PC) && defined(MELEE_NO_SAVES)
    if (mnSound_pc_lines[0] != NULL) {
        if (aurora_ax_output_status() == AURORA_AUDIO_OUTPUT_PENDING)
            return;
        if (events & MenuInput_Back) {
            sfxBack();
            mn_804A04F0.entering_menu = 0;
            gmMainLib_8015ED80(menu->unk3);
            mn_80229894(4, 1, 3);
            return;
        }
        if (events & (MenuInput_Up | MenuInput_Down)) {
            menu->unk2 = !menu->unk2;
            sfxMove();
            mnSound_pc_draw();
        } else if (events & (MenuInput_Left | MenuInput_Right)) {
            if (menu->unk2 == 0) {
                u32 channels = aurora_ax_requested_output_channels() == 6 ? 2 : 6;
                if (aurora_ax_request_output_channels(channels)) {
                    aurora_ax_set_stereo_upmix(channels == 6);
                    sfxMove();
                }
            } else {
                int value = (s8) menu->unk3 + ((events & MenuInput_Left) ? -5 : 5);
                if (value >= -100 && value <= 100) {
                    Vec3 pos_0, pos_1;
                    HSD_JObj *anim_0, *anim_1, *anim_2;
                    menu->unk3 = value;
                    updateVolumeAnim(GET_JOBJ(mnSound_804D6C30), menu->unk3,
                                     (events & MenuInput_Left) ? -1 : 1,
                                     &pos_0, &pos_1, &anim_0, &anim_1, &anim_2);
                    gmMainLib_8015ED80(menu->unk3);
                    sfxMove();
                }
            }
            mnSound_pc_draw();
        }
        return;
    }
#endif
    if (events & MenuInput_Back) {
        sfxBack();
        mn_804A04F0.entering_menu = 0;
        gmMainLib_8015ED80(((Menu*) mnSound_804D6C30->user_data)->unk3);
        lbCardGame_SaveChanges();
        mn_80229894(4, 1, 3);
        return;
    }
    if (events & (MenuInput_Up | MenuInput_Down)) {
        // switch between the two
        sfxMove();
        if (menu->unk2 == 0) {
            menu->unk2 = 1;
        } else {
            menu->unk2 = 0;
        }
        updateCenterText();
        return;
    }
    if (events & MenuInput_Left) {
        if (menu->unk2 == 0) {
            if (menu->unk1 == 1) {
                // switch from mono to stereo
                sfxMove();
                menu->unk1 = 0;
                {
                    HSD_JObj* jobj_anim_2;
                    HSD_JObj* jobj_anim_1;
                    HSD_JObj* jobj_anim_0;
                    PAD_STACK(4);
                    updateChannelAnim(GET_JOBJ(mnSound_804D6C30), menu->unk1,
                                      &jobj_anim_0, &jobj_anim_1,
                                      &jobj_anim_2);
                }
                lbAudioAx_80024C08(menu->unk1);
            }
        } else if ((s8) menu->unk3 > -100) {
            // move towards sounds
            sfxMove();
            menu->unk3 -= 5;
            mix = menu->unk3;
            {
                UNUSED u8 pad[4];
                Vec3 pos_1;
                Vec3 pos_0;
                HSD_JObj* jobj_anim_2;
                HSD_JObj* jobj_anim_1;
                HSD_JObj* jobj_anim_0;
                PAD_STACK(8);
                updateVolumeAnim(GET_JOBJ(mnSound_804D6C30), mix, -1, &pos_0,
                                 &pos_1, &jobj_anim_0, &jobj_anim_1,
                                 &jobj_anim_2);
            }
            gmMainLib_8015ED80(((Menu*) mnSound_804D6C30->user_data)->unk3);
        }
    } else if (events & MenuInput_Right) {
        if (menu->unk2 == 0) {
            if (menu->unk1 == 0) {
                // switch from stereo to mono
                sfxMove();
                menu->unk1 = 1;
                {
                    HSD_JObj* jobj_anim_2;
                    HSD_JObj* jobj_anim_1;
                    HSD_JObj* jobj_anim_0;
                    PAD_STACK(4);
                    updateChannelAnim(GET_JOBJ(mnSound_804D6C30), menu->unk1,
                                      &jobj_anim_0, &jobj_anim_1,
                                      &jobj_anim_2);
                }
                lbAudioAx_80024C08(menu->unk1);
            }
        } else if ((s8) menu->unk3 < 100) {
            // move towards music
            sfxMove();
            menu->unk3 += 5;
            mix = menu->unk3;
            {
                UNUSED u8 pad[4];
                Vec3 pos_1;
                Vec3 pos_0;
                HSD_JObj* jobj_anim_2;
                HSD_JObj* jobj_anim_1;
                HSD_JObj* jobj_anim_0;
                updateVolumeAnim(GET_JOBJ(mnSound_804D6C30), mix, 1, &pos_0,
                                 &pos_1, &jobj_anim_0, &jobj_anim_1,
                                 &jobj_anim_2);
            }
            gmMainLib_8015ED80(((Menu*) mnSound_804D6C30->user_data)->unk3);
        }
    }
}

static inline void animateSelectedChannel(HSD_GObj* gobj, HSD_JObj** jobj_out)
{
    AnimLoopSettings* anim = mnSound_803EEED8;
    Menu* menu = GET_MENU(gobj);
    HSD_JObj* jobj = GET_JOBJ(gobj);
    if (menu->unk2 == 0) {
        if (menu->unk1 == 0) {
            lb_80011E24(jobj, jobj_out, 0xA, -1);
        } else {
            lb_80011E24(jobj, jobj_out, 9, -1);
        }
        mn_8022EC18(*jobj_out, anim, 0x80);
    }
}

static inline void chooseVolumeAnim(HSD_JObj* jobj, AnimLoopSettings* settings)
{
    f32 cursor_frame = mn_8022F298(jobj);
    if ((volume_left.start_frame <= cursor_frame) &&
        (cursor_frame <= volume_left.end_frame))
    {
        *settings = volume_left;
    } else {
        *settings = volume_right;
    }
}

void fn_80249A1C(HSD_GObj* gobj)
{
    AnimLoopSettings* anim = mnSound_803EEED8;
    Menu* menu = GET_MENU(gobj);
    HSD_JObj* jobj = gobj->hsd_obj;
    HSD_JObj* sound_selection_jobj;
    HSD_JObj* channel_selection_jobj;
    AnimLoopSettings* temp;
    UNUSED u8 pad38[4];
    HSD_JObj* selected_channel_jobj;
    AnimLoopSettings volume_anim;
    UNUSED u8 pad24[4];
    HSD_JObj* volume_cursor_jobj;
    u8 cursor_timer;
    PAD_STACK(8);
#if defined(TARGET_PC) && defined(MELEE_NO_SAVES)
    if (mnSound_pc_lines[0] != NULL) {
        if (mn_804A04F0.cur_menu != MENU_KIND_SETTINGS_SOUND) {
            HSD_Text* owned_text = menu->text;
            mnSound_pc_destroy();
            HSD_SisLib_803A5CC4(owned_text);
            HSD_GObjFree(gobj);
            return;
        }
        HSD_JObjSetFlagsAll(jobj, JOBJ_HIDDEN);
        if (mnSound_pc_status != aurora_ax_output_status() ||
            mnSound_pc_effective != aurora_ax_output_channels())
            mnSound_pc_draw();
        return;
    }
#endif
    if (mn_804A04F0.cur_menu != MENU_KIND_SETTINGS_SOUND) {
        HSD_GObjFree(gobj);
        HSD_SisLib_803A5CC4(menu->text);
        return;
    }
    cursor_timer = menu->cursor;
    if (cursor_timer != 0) {
        menu->cursor = cursor_timer - 1;
        if (menu->cursor != 0) {
            HSD_JObjSetFlagsAll(jobj, JOBJ_HIDDEN);
            return;
        }
        HSD_JObjClearFlagsAll(jobj, JOBJ_HIDDEN);
        return;
    }
    animateSelectedChannel(gobj, &selected_channel_jobj);
    lb_80011E24(gobj->hsd_obj, &volume_cursor_jobj, 6, -1);
    chooseVolumeAnim(volume_cursor_jobj, &volume_anim);
    mn_8022ED6C(volume_cursor_jobj, &volume_anim);
    lb_80011E24(jobj, &sound_selection_jobj, 0xE, -1);
    {
        temp = &anim[menu->unk2];
        mn_8022ED6C(sound_selection_jobj, temp + 1);
    }
    lb_80011E24(jobj, &channel_selection_jobj, 0xB, -1);
    {
        AnimLoopSettings* temp = &anim[menu->unk1];
        mn_8022ED6C(channel_selection_jobj, temp + 3);
    }
}

static inline void initUserData(Menu* user_data, HSD_GObj* gobj)
{
    HSD_GObjProc* proc;
    user_data->unk3 = gmMainLib_8015ED74();
    user_data->text = NULL;
    GObj_InitUserData(gobj, 0U, HSD_Free, user_data);
    proc = HSD_GObj_SetupProc(gobj, fn_80249A1C, 0U);
    proc->flags_3 = HSD_GObj_804D783C;
}

void mnSound_80249C08(int unused)
{
    StaticModelDesc* model = &mnSound_804A08A8;
    AnimLoopSettings* anims = mnSound_803EEED8;
    HSD_JObj* sound_selection_jobj;
    HSD_JObj* channel_selection_jobj;
    HSD_JObj* volume_cursor_jobj;
    HSD_JObj* jobj;
    Menu* user_data;
    UNUSED HSD_GObjProc* proc;
    HSD_GObj* gobj = GObj_Create(HSD_GOBJ_CLASS_ITEM, 7U, 0x80U);
    mnSound_804D6C30 = gobj;
    jobj = HSD_JObjLoadJoint(model->joint);
    HSD_GObjObject_80390A70(gobj, HSD_GObj_JObjKind, jobj);
    GObj_SetupGXLink(gobj, HSD_GObj_JObjCallback, 4U, 0x80);
    HSD_JObjAddAnimAll(jobj, model->animjoint, model->matanim_joint,
                       model->shapeanim_joint);
    HSD_JObjReqAnimAll(jobj, 0.0F);
    user_data = HSD_MemAlloc(sizeof(Menu));
    HSD_ASSERTREPORT(0x22CU, user_data, "Can't get user_data.\n");
    gmMainLib_GetGameRules();
    user_data->cursor = 0x14;
    user_data->unk1 = lbAudioAx_80024BD0();
    user_data->unk2 = 0U;
    initUserData(user_data, gobj);

    {
        Menu* menu = GET_MENU(mnSound_804D6C30);
        if (menu->text != NULL) {
            HSD_SisLib_803A5CC4(menu->text);
        }
        Menu_InitCenterText(menu, menu->unk2 == 0 ? 0xBB : 0xBC);
    }

    {
        HSD_JObj* channel_anim_2;
        HSD_JObj* channel_anim_1;
        HSD_JObj* channel_anim_0;
        PAD_STACK(4);
        updateChannelAnim(GET_JOBJ(gobj), user_data->unk1, &channel_anim_0,
                          &channel_anim_1, &channel_anim_2);
    }

    {
        lb_80011E24(jobj, &volume_cursor_jobj, 6, -1);
        HSD_JObjReqAnimAll(volume_cursor_jobj, volume_left.end_frame);
        HSD_JObjAnimAll(volume_cursor_jobj);
    }

    {
        Vec3 pos_1;
        Vec3 pos_0;
        HSD_JObj* jobj_anim_2;
        HSD_JObj* jobj_anim_1;
        HSD_JObj* jobj_anim_0;
        PAD_STACK(4);
        updateVolumeAnim(GET_JOBJ(gobj), user_data->unk3, 0, &pos_0, &pos_1,
                         &jobj_anim_0, &jobj_anim_1, &jobj_anim_2);
    }

    {
        lb_80011E24(jobj, &sound_selection_jobj, 0xE, -1);
        {
            AnimLoopSettings* anim = &anims[user_data->unk2];
            mn_8022ED6C(sound_selection_jobj, anim + 1);
        }
        lb_80011E24(jobj, &channel_selection_jobj, 0xB, -1);
        HSD_JObjReqAnimAll(channel_selection_jobj,
                           anims[user_data->unk1 + 3].end_frame);
        HSD_JObjAnimAll(channel_selection_jobj);
        HSD_JObjSetFlagsAll(jobj, JOBJ_HIDDEN);
    }
#if defined(TARGET_PC) && defined(MELEE_NO_SAVES)
    /* Keep the original front-bus source mix in stereo. Output layout is a
     * separate native preference, not a false new console sound-mode value. */
    lbAudioAx_80024C08(0);
    mnSound_pc_draw();
#endif
}

void mnSound_8024A09C(int arg0)
{
    HSD_GObjProc* proc;
    HSD_Archive* archive;

    mn_804D6BC8.cooldown = 5;
    mn_804A04F0.prev_menu = mn_804A04F0.cur_menu;
    mn_804A04F0.cur_menu = MENU_KIND_SETTINGS_SOUND;
    mn_804A04F0.hovered_selection = 0;
    archive = mn_804D6BB8;
    lbArchive_LoadSections(
        archive, &mnSound_804A08A8.joint, "MenMainConSo_Top_joint",
        &mnSound_804A08A8.animjoint, "MenMainConSo_Top_animjoint",
        &mnSound_804A08A8.matanim_joint, "MenMainConSo_Top_matanim_joint",
        &mnSound_804A08A8.shapeanim_joint, "MenMainConSo_Top_shapeanim_joint",
        0);
    mnSound_80249C08(arg0);
    proc = HSD_GObj_SetupProc(GObj_Create(0, 1, 0x80), mnSound_802492CC, 0);
    proc->flags_3 = HSD_GObj_804D783C;
}
