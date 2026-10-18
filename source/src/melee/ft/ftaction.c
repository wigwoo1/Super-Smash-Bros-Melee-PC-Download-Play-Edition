#include "ftaction.h"
#include <pc/ft_action_fields.h>
#include <pc/fighter_command_dispatch.h>
#include <pc/fighter_packed_fields.h>
#ifdef TARGET_PC
#include <sysdolphin/baselib/debug.h>
#include <stdlib.h>
#include <dolphin/os.h>
#endif

#include <Runtime/platform.h>

#include <melee/lb/forward.h>

#include <placeholder.h>

#include "fighter.h"
#include "forward.h"
#include "ft_081B.h"
#include "ft_0877.h"
#include "ft_0881.h"
#include "ft_0899.h"
#include "ft_0C88.h"
#include "ft_0DF0.h"
#include "ftanim.h"
#include "ftcolanim.h"
#include "ftcoll.h"
#include "ftcommon.h"
#include "ftdynamics.h"
#include "ftparts.h"
#include "kinds/ftCommon/ftCo_09F7.h"
#include "types.h"
#include <dolphin/mtx.h>
#include <melee/lb/inlines.h>
#include <melee/lb/lbaudio_ax.h>
#include <melee/lb/lbcommand.h>
#include <melee/lb/types.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjproc.h>
#include <sysdolphin/baselib/random.h>

/* 07121C */ static void ftAction_8007121C(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 0715EC */ static void ftAction_800715EC(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 07162C */ static void ftAction_8007162C(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 07168C */ static void ftAction_8007168C(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 07169C */ static void ftAction_8007169C(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 0716F8 */ static void ftAction_800716F8(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 071708 */ static void ftAction_80071708(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 071774 */ static void ftAction_80071774(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 071784 */ static void ftAction_80071784(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 0717C8 */ static void ftAction_800717C8(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 0717D8 */ static void ftAction_800717D8(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 071810 */ static void ftAction_80071810(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 071820 */ static void ftAction_80071820(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 0718A4 */ static void ftAction_800718A4(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 071908 */ static void ftAction_80071908(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 07192C */ static void ftAction_8007192C(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 071950 */ static void ftAction_80071950(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 071974 */ static void ftAction_80071974(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 071998 */ static void ftAction_80071998(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 071A14 */ static void ftAction_80071A14(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 071A58 */ static void ftAction_80071A58(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 071A9C */ static void ftAction_80071A9C(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 071AE8 */ static void ftAction_80071AE8(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 071B28 */ static void ftAction_80071B28(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 071CCC */ static void ftAction_80071CCC(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 071D30 */ static void ftAction_80071D30(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 071D40 */ static void ftAction_80071D40(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 071D94 */ static void ftAction_80071D94(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 071DCC */ static void ftAction_80071DCC(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 071E04 */ static void ftAction_80071E04(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 071F0C */ static void ftAction_80071F0C(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 071F34 */ static void ftAction_80071F34(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 071F78 */ static void ftAction_80071F78(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 071FA0 */ static void ftAction_80071FA0(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 071FC8 */ static void ftAction_80071FC8(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 0722C8 */ static void ftAction_800722C8(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 072320 */ static void ftAction_80072320(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 0726C0 */ static void ftAction_800726C0(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 0726F4 */ static void ftAction_800726F4(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 0727C8 */ static void ftAction_800727C8(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 07283C */ static void ftAction_8007283C(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 072894 */ static void ftAction_80072894(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 0728F8 */ static void ftAction_800728F8(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 07296C */ static void ftAction_8007296C(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 07297C */ static void ftAction_8007297C(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 0729C4 */ static void ftAction_800729C4(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 0729D4 */ static void ftAction_800729D4(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 072A4C */ static void ftAction_80072A4C(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 072A5C */ static void ftAction_80072A5C(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 072AAC */ static void ftAction_80072AAC(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 072ABC */ static void ftAction_80072ABC(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 072B04 */ static void ftAction_80072B04(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 072B14 */ static void ftAction_80072B14(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 072B3C */ static void ftAction_80072B3C(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 072B84 */ static void ftAction_80072B84(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 072B94 */ static void ftAction_80072B94(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 072BE4 */ static void ftAction_80072BE4(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 072BF4 */ static void ftAction_80072BF4(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 072C5C */ static void ftAction_80072C5C(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 072C6C */ static void ftAction_80072C6C(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 072CB0 */ static void ftAction_80072CB0(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 072CD8 */ static void ftAction_80072CD8(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 072E24 */ static void ftAction_80072E24(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 072E4C */ static void ftAction_80072E4C(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 072FE0 */ static void ftAction_80072FE0(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 073008 */ static void ftAction_80073008(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 07309C */ static void ftAction_8007309C(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 073118 */ static void ftAction_80073118(Fighter_GObj* gobj,
                                           CommandInfo* cmd);
/* 07320C */ static void ftAction_8007320C(Fighter_GObj* gobj,
                                           CommandInfo* cmd);

#ifdef TARGET_PC
/* Color callbacks use native scalar command words. Animation streams use
 * packed PPC bytes; bridge just the shared gfx/sound callbacks locally. */
static void ftAction_RunNativePacked(FtCmd callback, Fighter_GObj* gobj,
                                     CommandInfo* cmd, unsigned count)
{
    u32 words[5];
    CommandInfo scalar = *cmd;
    HSD_ASSERT(0, count <= ARRAY_SIZE(words));
    for (unsigned i = 0; i < count; ++i) {
        words[i] = FT_PACKED_WORD(cmd, i * 4);
    }
    scalar.x8.u = (CmdUnion*) words;
    callback(gobj, &scalar);
    ptrdiff_t consumed = (u8*) scalar.x8.u - (u8*) words;
    HSD_ASSERT(0, consumed >= 0 && consumed % 4 == 0 &&
                      (unsigned) consumed <= count * 4);
    cmd->x8.u += consumed / 4;
}
static void ftAction_GfxPacked(Fighter_GObj* gobj, CommandInfo* cmd)
{
    ftAction_RunNativePacked(ftAction_80071028, gobj, cmd, 5);
}
static void ftAction_SoundPacked(Fighter_GObj* gobj, CommandInfo* cmd)
{
    ftAction_RunNativePacked(ftAction_80071B50, gobj, cmd, 3);
}
#else
#define ftAction_GfxPacked ftAction_80071028
#define ftAction_SoundPacked ftAction_80071B50
#endif

static FtCmd ftAction_803C06E8[] = {
    ftAction_GfxPacked, ftAction_8007121C, ftAction_8007162C, ftAction_8007169C,
    ftAction_80071708, ftAction_80071784, ftAction_800717D8, ftAction_SoundPacked,
    ftAction_80071CCC, ftAction_80071820, ftAction_800718A4, ftAction_80071908,
    ftAction_8007192C, ftAction_80071950, ftAction_80071974, ftAction_80071998,
    ftAction_80071A14, ftAction_80071A58, ftAction_80071A9C, ftAction_80071AE8,
    ftAction_80071B28, ftAction_80071D40, ftAction_80071D94, ftAction_80071DCC,
    ftAction_80071E04, ftAction_80071F34, ftAction_80071F78, ftAction_80071FA0,
    ftAction_80071FC8, ftAction_80072320, ftAction_800726F4, ftAction_800727C8,
    ftAction_80072894, ftAction_800728F8, ftAction_8007297C, ftAction_800729D4,
    ftAction_80072A5C, ftAction_80072ABC, ftAction_80072B14, ftAction_80072B3C,
    ftAction_80072B94, ftAction_80072BF4, ftAction_80072C6C, ftAction_80072CB0,
    ftAction_80072CD8, ftAction_80072E4C, ftAction_80073008, ftAction_800730B8,
    ftAction_80073118,
};

static FtCmd ftAction_803C07AC[ARRAY_SIZE(ftAction_803C06E8)] = {
    ftAction_800711DC, ftAction_800715EC, ftAction_8007168C, ftAction_800716F8,
    ftAction_80071774, ftAction_800717C8, ftAction_80071810, ftAction_80071CA4,
    ftAction_80071D30, ftAction_80071820, ftAction_800718A4, ftAction_80071908,
    ftAction_8007192C, ftAction_80071950, ftAction_80071974, ftAction_80071998,
    ftAction_80071A14, ftAction_80071A58, ftAction_80071A9C, ftAction_80071AE8,
    ftAction_80071B28, ftAction_80071D40, ftAction_80071D94, ftAction_80071DCC,
    ftAction_80071F0C, ftAction_80071F34, ftAction_80071F78, ftAction_80071FA0,
    ftAction_800722C8, ftAction_800726C0, ftAction_800726F4, ftAction_8007283C,
    ftAction_80072894, ftAction_8007296C, ftAction_800729C4, ftAction_80072A4C,
    ftAction_80072AAC, ftAction_80072B04, ftAction_80072B14, ftAction_80072B84,
    ftAction_80072BE4, ftAction_80072C5C, ftAction_80072C6C, ftAction_80072CB0,
    ftAction_80072E24, ftAction_80072FE0, ftAction_8007309C, ftAction_80073108,
    ftAction_8007320C,
};

static u8 ftAction_803C0870[ARRAY_SIZE(ftAction_803C06E8)] = {
    05, 05, 01, 01, 01, 01, 01, 03, 01, 01, 01, 01, 01, 01, 01, 01, 01,
    01, 01, 01, 01, 01, 01, 01, 03, 01, 01, 01, 07, 04, 01, 01, 01, 01,
    01, 01, 01, 01, 01, 01, 01, 01, 01, 01, 03, 03, 02, 01, 04
};

/*
SubactionEvent 10 GFXSpawn

https://github.com/Ploaj/HSDLib/blob/939cc10be5a9f76fdcf3ca8ff4a54e35e545fe4a/HSDRawViewer/Scripts/command_fighter.yml#L35

Bone ID
Use Common Bone IDs
Destroy on State Change
Unk1
GFX ID
Unk2
Offset Z
Offset Y
Offset X
Range Z
Range Y
Range X
*/

/// @brief Spawns GFX
void ftAction_80071028(Fighter_GObj* gobj, CommandInfo* cmd)
{
    Fighter* fp = GET_FIGHTER(gobj);
    float unk;
    Vec3 offset;
    Vec3 range;
    int bone;
    int use_common_bone_id;
    int destroy_on_state_change;
    u32 gfx_id;
    {
        if (!fp->invisible) {
            if (FT_ACTION_U(cmd,spawn_gfx_0.useUnkBone,15,1U)) {
                bone = fp->ft_data->x8->x12;
            } else {
                bone = FT_ACTION_U(cmd,spawn_gfx_0.boneId,18,255U);
            }
            use_common_bone_id = FT_ACTION_U(cmd,spawn_gfx_0.useCommonBoneIDs,17,1U);
            destroy_on_state_change =
                FT_ACTION_U(cmd,spawn_gfx_0.destroyOnStateChange,16,1U);

            NEXT_CMD(cmd);
            gfx_id = FT_ACTION_U(cmd,spawn_gfx_1.gfxID,16,65535U);
            unk = FT_ACTION_U(cmd,spawn_gfx_1.unkFloat,0,65535U);

            NEXT_CMD(cmd);
            /// @todo i believe they are actually read in reverse order, maybe
            // ftCo_8009F834 also reads them in reverse.
            // double check this in-game eventually...
            offset.x = 0.003906f * FT_ACTION_S16(cmd,spawn_gfx_2.offsetZ,16);
            offset.y = 0.003906f * FT_ACTION_S16(cmd,spawn_gfx_2.offsetY,0);

            NEXT_CMD(cmd);
            offset.z = 0.003906f * FT_ACTION_S16(cmd,spawn_gfx_3.offsetX,16);
            range.x = 0.003906f * FT_ACTION_U(cmd,spawn_gfx_3.rangeZ,0,65535U);

            NEXT_CMD(cmd);
            range.y = 0.003906f * FT_ACTION_U(cmd,spawn_gfx_4.rangeY,16,65535U);
            range.z = 0.003906f * FT_ACTION_U(cmd,spawn_gfx_4.rangeX,0,65535U);

            NEXT_CMD(cmd);
            ftCo_8009F834(gobj, gfx_id, bone, use_common_bone_id,
                          destroy_on_state_change, &offset, &range, unk);
        } else {
            ftAction_800711DC(gobj, cmd);
        }
    }
}

/// @brief Skips GFX Spawn
void ftAction_800711DC(Fighter_GObj* gobj, CommandInfo* cmd)
{
    SKIP_CMD(cmd, 5);
}

/// @brief Spawns Hitboxes
void ftAction_8007121C(Fighter_GObj* gobj, CommandInfo* cmd)
{
    Fighter* fp;
    HitCapsule* hitbox;
    u32 hit_group;
    u32 idx;
    PAD_STACK(8);

    fp = GET_FIGHTER(gobj);
    /// @todo this matches but isnt pretty. maybe an inline/macro as
    // we dont have enough stack in general?
#ifdef TARGET_PC
    /* Original PPC lbz at 0x80071248 reads byte15 bit3. */
    unsigned skip_thrown = (FT_PACKED_WORD(cmd, 12) >> 3) & 1;
#else
    unsigned skip_thrown = ((struct spawn_hitbox_skip*) cmd->x8.u)->xF_b4;
#endif
    if (skip_thrown && (fp->x1064_thrownHitbox.x134.owner == NULL)) {
        ftAction_800715EC(gobj, cmd);
    } else {
        hit_group = FT_PACKED_U(cmd,create_hitbox_0.hit_group,20,0x7U);
#ifdef TARGET_PC
        HSD_ASSERT(0, FT_PACKED_U(cmd,create_hitbox_0.id,23,0x7U) <
                          ARRAY_SIZE(fp->x914));
#endif
        if (((hitbox = &fp->x914[FT_PACKED_U(cmd,create_hitbox_0.id,23,0x7U)])->state ==
             HitCapsule_Disabled) ||
            (hitbox->x4 != hit_group))
        {
            hitbox->x4 = hit_group;
            hitbox->state = HitCapsule_Enabled;
            fp->x2219_b3 = 1;
            ftColl_800768A0(fp, hitbox);
        }
        idx = FT_PACKED_U(cmd,create_hitbox_0.bone,11,0xffU);
#ifdef TARGET_PC
        if (getenv("MELEE_DIAGNOSTIC_HIT_JOINT_TRACE")) {
            OSReport("NATIVE_HIT_JOINT assign fighter=%p kind=%d motion=%d anim=%d capsule=%p command=%p word=%08x bone=%u common=%u parts=%p count=%u\n",
                     fp, fp->kind, fp->motion_id, fp->anim_id, hitbox,
                     cmd->x8.u, FT_PACKED_WORD(cmd, 0), idx,
                     FT_PACKED_U(cmd,create_hitbox_0.use_common_bone_ids,10,0x1U),
                     fp->parts, ftPartsTable[fp->kind]->parts_num);
        }
#endif
        if (FT_PACKED_U(cmd,create_hitbox_0.use_common_bone_ids,10,0x1U)) {
            hitbox->jobj = fp->parts[ftParts_GetBoneIndex(
                                         fp, FT_PACKED_U(cmd,create_hitbox_0.bone,11,0xffU))]
                               .joint;
        } else {
            hitbox->jobj = fp->parts[idx].joint;
        }
#ifdef TARGET_PC
        if (getenv("MELEE_DIAGNOSTIC_HIT_JOINT_TRACE"))
            OSReport("NATIVE_HIT_JOINT assigned fighter=%p capsule=%p joint=%p\n",
                     fp, hitbox, hitbox->jobj);
#endif
        ftColl_8007ABD0(hitbox, FT_PACKED_U(cmd,create_hitbox_0.damage,0,0x3ffU), gobj);
        NEXT_CMD(cmd);
        hitbox->scale = 0.003906f * FT_PACKED_U(cmd,create_hitbox_1.size,16,0xffffU);
        hitbox->b_offset.x = 0.003906f * FT_PACKED_S(cmd,create_hitbox_1.z_offset,0,16);
        NEXT_CMD(cmd);
        hitbox->b_offset.y = 0.003906f * FT_PACKED_S(cmd,create_hitbox_2.y_offset,16,16);
        hitbox->b_offset.z = 0.003906f * FT_PACKED_S(cmd,create_hitbox_2.x_offset,0,16);
        NEXT_CMD(cmd);
        ftColl_8007AC9C(hitbox, FT_PACKED_U(cmd,create_hitbox_3.angle,23,0x1ffU), gobj);
        hitbox->x24 = FT_PACKED_U(cmd,create_hitbox_3.knockback_growth,14,0x1ffU);
        hitbox->x28 = FT_PACKED_U(cmd,create_hitbox_3.weight_set_knockback,5,0x1ffU);
        hitbox->x43_b0 = FT_PACKED_U(cmd,create_hitbox_3.item_hit_interaction,4,0x1U);
        hitbox->x43_b1 = FT_PACKED_U(cmd,create_hitbox_3.ignore_fighter_scale,2,0x1U);
        hitbox->x40_b0 = FT_PACKED_U(cmd,create_hitbox_3.clank,1,0x1U);
        hitbox->x40_b1 = FT_PACKED_U(cmd,create_hitbox_3.rebound,0,0x1U);
        NEXT_CMD(cmd);
        hitbox->x2C = FT_PACKED_U(cmd,create_hitbox_4.base_knockback,23,0x1ffU);
        hitbox->element = FT_PACKED_U(cmd,create_hitbox_4.element,18,0x1fU);
        hitbox->x34 = FT_PACKED_S(cmd,create_hitbox_4.shield_damage,10,8);
        hitbox->sfx_severity = FT_PACKED_U(cmd,create_hitbox_4.hit_sfx_severity,7,0x7U);
        hitbox->sfx_kind = FT_PACKED_U(cmd,create_hitbox_4.hit_sfx_kind,2,0x1fU);
        hitbox->x40_b2 = FT_PACKED_U(cmd,create_hitbox_4.hit_aerial,0,0x1U);
        hitbox->x40_b3 = FT_PACKED_U(cmd,create_hitbox_4.hit_grounded,1,0x1U);
        NEXT_CMD(cmd);
        hitbox->x42_b5 = 1;
        hitbox->x42_b7 = 1;
        hitbox->x41_b4 = 0;
        hitbox->x41_b6 = 0;
        hitbox->x41_b5 = 0;
        hitbox->x42_b0 = 0;
        hitbox->x42_b4 = 0;
        hitbox->x41_b7 = 0;
        /* Original PPC reads following-word byte1 after five advances. */
        hitbox->x134.hit_grabbed_victim_only =
            FT_PACKED_U(cmd,create_hitbox_5.x1_b4,19,0x1U);
        hitbox->x42_b1 = 1;
        hitbox->x42_b2 = 0;
        hitbox->x43_b2 = 0;
        if ((HSD_GObj_CurrentInvokedProc != NULL) &&
            (HSD_GObj_CurrentInvokedProc->s_link > 9))
        {
            ftColl_8007AD18(fp, hitbox);
        }
    }
    ftCommon_80080484(fp);
}

/// @brief Skips Hitbox Spawn
void ftAction_800715EC(Fighter_GObj* gobj, CommandInfo* cmd)
{
    SKIP_CMD(cmd, 5);
}

/*
- code: 12
  name: Adjust Hitbox Damage
  parameters:
  - name: Hitbox ID
    bitCount: 3
  - name: Damage
    bitCount: 23
*/
/// @brief Adjust Hitbox Damage
void ftAction_8007162C(Fighter_GObj* gobj, CommandInfo* cmd)
{
    Fighter* fp = GET_FIGHTER(gobj);
    u32 idx = FT_PACKED_U(cmd,set_hitbox_damage.idx,23,7U);
#ifdef TARGET_PC
    HSD_ASSERT(0, idx < ARRAY_SIZE(fp->x914));
#endif
    HitCapsule* hit = &fp->x914[idx];
    ftColl_8007ABD0(hit, FT_PACKED_U(cmd,set_hitbox_damage.value,0,0x7fffffU), gobj);
    NEXT_CMD(cmd);
}

/// @remarks after each CommandInfo function, there seems to be
/// a function that has the ability to skip it (ftAction_800715EC's usage)
/// unsure if theyre all actually called
void ftAction_8007168C(Fighter_GObj* gobj, CommandInfo* cmd)
{
    SKIP_CMD(cmd, 1);
}

/// @brief Adjusts Hitbox Scale
void ftAction_8007169C(Fighter_GObj* gobj, CommandInfo* cmd)
{
    Fighter* fp = GET_FIGHTER(gobj);
    u32 idx = FT_PACKED_U(cmd,set_hitbox_scale.idx,23,7U);
#ifdef TARGET_PC
    HSD_ASSERT(0, idx < ARRAY_SIZE(fp->x914));
#endif
    HitCapsule* hit = &fp->x914[idx];
    hit->scale = 0.003906f * FT_PACKED_U(cmd,set_hitbox_scale.value,0,0x7fffffU);
    NEXT_CMD(cmd);
}

void ftAction_800716F8(Fighter_GObj* gobj, CommandInfo* cmd)
{
    SKIP_CMD(cmd, 1);
}

/// @brief Sets Hitbox flags
void ftAction_80071708(Fighter_GObj* gobj, CommandInfo* cmd)
{
    Fighter* fp = GET_FIGHTER(gobj);
    u32 idx = FT_PACKED_U(cmd,set_hitbox_x42_b57.idx,2,0xffffffU);
#ifdef TARGET_PC
    HSD_ASSERT(0, idx < ARRAY_SIZE(fp->x914));
#endif
    HitCapsule* hit = &fp->x914[idx];
    switch (FT_PACKED_U(cmd,set_hitbox_x42_b57.type,1,1U)) {
    case 0:
        hit->x42_b5 = FT_PACKED_U(cmd,set_hitbox_x42_b57.value,0,1U);
        break;
    case 1:
        hit->x42_b7 = FT_PACKED_U(cmd,set_hitbox_x42_b57.value,0,1U);
        break;
    }
    NEXT_CMD(cmd);
}

void ftAction_80071774(Fighter_GObj* gobj, CommandInfo* cmd)
{
    SKIP_CMD(cmd, 1);
}

void ftAction_80071784(Fighter_GObj* gobj, CommandInfo* cmd)
{
    ftColl_8007AFC8(gobj, FT_PACKED_U(cmd,set_throw_flags.hit_idx,0,0x3ffffffU));
    NEXT_CMD(cmd);
}

void ftAction_800717C8(Fighter_GObj* gobj, CommandInfo* cmd)
{
    SKIP_CMD(cmd, 1);
}

void ftAction_800717D8(Fighter_GObj* gobj, CommandInfo* cmd)
{
    ftColl_8007AFF8(gobj);
    NEXT_CMD(cmd);
}

void ftAction_80071810(Fighter_GObj* gobj, CommandInfo* cmd)
{
    SKIP_CMD(cmd, 1);
}

/// Set one of #Fighter::cmd_vars
/// @todo Heavily suggests that #Fighter::cmd_vars is not an array.
void ftAction_80071820(Fighter_GObj* gobj, CommandInfo* cmd)
{
    Fighter* fp = GET_FIGHTER(gobj);

    switch (FT_PACKED_U(cmd,set_cmd_var.idx,24,3U)) {
    case 0:
        fp->cmd_vars[0] = FT_PACKED_U(cmd,set_cmd_var.value,0,0xffffffU);
        break;
    case 1:
        fp->cmd_vars[1] = FT_PACKED_U(cmd,set_cmd_var.value,0,0xffffffU);
        break;
    case 2:
        fp->cmd_vars[2] = FT_PACKED_U(cmd,set_cmd_var.value,0,0xffffffU);
        break;
    case 3:
        fp->cmd_vars[3] = FT_PACKED_U(cmd,set_cmd_var.value,0,0xffffffU);
        break;
    }
    NEXT_CMD(cmd);
}

void ftAction_800718A4(Fighter_GObj* gobj, CommandInfo* cmd)
{
    Fighter* fp = GET_FIGHTER(gobj);
    switch (FT_PACKED_U(cmd,set_throw_flags.hit_idx,0,0x3ffffffU)) {
    case 0:
        fp->x2210.x0.throw_flags_b3 = true;
        fp->cmd_timer = cmd->timer;
        break;
    case 1:
        fp->x2210.x0.throw_flags_b4 = true;
        break;
    }
    NEXT_CMD(cmd);
}

void ftAction_80071908(Fighter_GObj* gobj, CommandInfo* cmd)
{
    Fighter* fp = GET_FIGHTER(gobj);
    fp->x2210.x0.throw_flags_b1 = true;
    NEXT_CMD(cmd);
}

void ftAction_8007192C(Fighter_GObj* gobj, CommandInfo* cmd)
{
    Fighter* fp = GET_FIGHTER(gobj);
    fp->x2210.x0.throw_flags_b2 = true;
    NEXT_CMD(cmd);
}

/// Allow interrupt
void ftAction_80071950(Fighter_GObj* gobj, CommandInfo* cmd)
{
    Fighter* fp = GET_FIGHTER(gobj);
    fp->allow_interrupt = true;
    NEXT_CMD(cmd);
}

void ftAction_80071974(Fighter_GObj* gobj, CommandInfo* cmd)
{
    Fighter* fp = GET_FIGHTER(gobj);
    fp->x2210.x0.throw_flags_b0 = true;
    NEXT_CMD(cmd);
}

void ftAction_80071998(Fighter_GObj* gobj, CommandInfo* cmd)
{
    Fighter* fp = GET_FIGHTER(gobj);
    switch (FT_PACKED_U(cmd,set_airborne_state.state,0,0x3ffffffU)) {
    case 0:
        ftCommon_8007D7FC(fp);
        break;
    case 1:
        ftCommon_8007D5D4(fp);
        break;
    case 2:
        ftCommon_8007D60C(fp);
        break;
    }
    NEXT_CMD(cmd);
}

void ftAction_80071A14(Fighter_GObj* gobj, CommandInfo* cmd)
{
    ftColl_8007B62C(gobj, FT_PACKED_U(cmd,set_airborne_state.state,0,0x3ffffffU));
    NEXT_CMD(cmd);
}

void ftAction_80071A58(Fighter_GObj* gobj, CommandInfo* cmd)
{
    ftColl_8007B0C0(gobj, FT_PACKED_U(cmd,set_airborne_state.state,0,0x3ffffffU));
    NEXT_CMD(cmd);
}

/// @brief Sets Hurt Capsule State
void ftAction_80071A9C(Fighter_GObj* gobj, CommandInfo* cmd)
{
    ftColl_8007B128(gobj, FT_PACKED_U(cmd,set_hurt_state.bone_idx,18,255U),
                    FT_PACKED_U(cmd,set_hurt_state.state,0,0x3ffffU));
    NEXT_CMD(cmd);
}

/// @brief Sets Jab Combo
void ftAction_80071AE8(Fighter_GObj* gobj, CommandInfo* cmd)
{
    Fighter* fp = GET_FIGHTER(gobj);

    if (!FT_PACKED_U(cmd,set_jab_combo.disabled,0,0x3ffffffU) || fp->x197C != NULL) {
        fp->x2218_b1 = true;
    }
    NEXT_CMD(cmd);
}

void ftAction_80071B28(Fighter_GObj* gobj, CommandInfo* cmd)
{
    Fighter* fp = GET_FIGHTER(gobj);
    fp->x2218_b2 = FT_PACKED_U(cmd,set_jab_rapid.state,0,0x3ffffffU);
    NEXT_CMD(cmd);
}

void ftAction_80071B50(Fighter_GObj* gobj, CommandInfo* cmd)
{
    Fighter* fp;
    u32 sfx;
    u8 vol;
    u8 pan;
    s32 behavior;

    fp = GET_FIGHTER(gobj);
    behavior = FT_ACTION_U(cmd,sound_effect_0.behavior,18,255U);
    NEXT_CMD(cmd);

    switch (behavior) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
        sfx = FT_ACTION_U(cmd,sound_effect_1.sfx_id,0,0xffffffffU);
        NEXT_CMD(cmd);
        vol = FT_ACTION_U(cmd,sound_effect_2.volume,8,255U);
        pan = FT_ACTION_U(cmd,sound_effect_2.panning,0,255U);

        switch (behavior) {
        case 0:
            ft_PlaySFX(fp, sfx, vol, pan);
            break;
        case 1:
            ft_80088478(fp, sfx, vol, pan);
            break;
        case 2:
            ft_800881D8(fp, sfx, vol, pan);
            break;
        case 3:
            ft_80088510(fp, sfx, vol, pan);
            break;
        case 4:
            ft_800885A8(fp, sfx, vol, pan);
            break;
        case 5:
            ft_80088640(fp, sfx, vol, pan);
            break;
        case 6:
            ft_80088328(fp, sfx, vol, pan);
            break;
        }
        break;

    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
    case 15:
        NEXT_CMD(cmd);
        switch (behavior) {
        case 10:
            ft_80088828(fp);
            break;
        case 11:
            ft_80088770(fp);
            break;
        case 12:
            ft_80088884(fp);
            break;
        case 13:
            ft_800888E0(fp);
            break;
        case 14:
            ft_8008893C(fp);
            break;
        case 15:
            ft_800887CC(fp);
            break;
        }
        break;

    default:
        break;
    }

    NEXT_CMD(cmd);
}

void ftAction_80071CA4(Fighter_GObj* gobj, CommandInfo* cmd)
{
    SKIP_CMD(cmd, 3);
}

void ftAction_80071CCC(Fighter_GObj* gobj, CommandInfo* cmd)
{
    Fighter* fp = GET_FIGHTER(gobj);
    FtSFX* sfx = fp->ft_data->x4C_sfx;
    FtSFXArr* sfx_smash = sfx->smash;
    if (sfx_smash == NULL) {
        NEXT_CMD(cmd);
    } else {
        ft_800889F4(fp, sfx_smash);
        NEXT_CMD(cmd);
    }
}

void ftAction_80071D30(Fighter_GObj* gobj, CommandInfo* cmd)
{
    SKIP_CMD(cmd, 1);
}

void ftAction_80071D40(Fighter_GObj* gobj, CommandInfo* cmd)
{
    ftParts_80074B0C(gobj, FT_PACKED_S(cmd,set_dobj_flags.idx,19,7),
                     FT_PACKED_S(cmd,set_dobj_flags.value,0,19));
    NEXT_CMD(cmd);
}

void ftAction_80071D94(Fighter_GObj* gobj, CommandInfo* cmd)
{
    ftParts_80074A8C(gobj);
    NEXT_CMD(cmd);
}

void ftAction_80071DCC(Fighter_GObj* gobj, CommandInfo* cmd)
{
    ftParts_80074ACC(gobj);
    NEXT_CMD(cmd);
}

void ftAction_80071E04(Fighter_GObj* gobj, CommandInfo* cmd)
{
    Fighter* fp = GET_FIGHTER(gobj);
    HitCapsule* hit = &fp->xDF4[FT_PACKED_U(cmd,set_throw_hitbox_0.idx,23,0x7U)];
    ftColl_8007ABD0(hit, FT_PACKED_U(cmd,set_throw_hitbox_0.damage,0,0x7fffffU), gobj);
    NEXT_CMD(cmd);

    ftColl_8007AC9C(hit, FT_PACKED_U(cmd,set_throw_hitbox_1.unk0,23,0x1ffU), gobj);
    hit->x24 = FT_PACKED_U(cmd,set_throw_hitbox_1.hit_x24,14,0x1ffU);
    hit->x28 = FT_PACKED_U(cmd,set_throw_hitbox_1.hit_x28,5,0x1ffU);
    NEXT_CMD(cmd);

    hit->x2C = FT_PACKED_U(cmd,set_throw_hitbox_2.hit_x2C,23,0x1ffU);
    hit->element = FT_PACKED_U(cmd,set_throw_hitbox_2.element,19,0xfU);
    hit->sfx_severity = FT_PACKED_U(cmd,set_throw_hitbox_2.sfx_severity,16,0x7U);
    hit->sfx_kind = FT_PACKED_U(cmd,set_throw_hitbox_2.sfx_kind,12,0xfU);
    NEXT_CMD(cmd);
}

void ftAction_80071F0C(Fighter_GObj* gobj, CommandInfo* cmd)
{
    SKIP_CMD(cmd, 3);
}

void ftAction_80071F34(Fighter_GObj* gobj, CommandInfo* cmd)
{
    PAD_STACK(8);
    ftCommon_8007F5CC(gobj, FT_PACKED_U(cmd,unk27.value,0,0x3ffffffU));
    NEXT_CMD(cmd);
}

void ftAction_80071F78(Fighter_GObj* gobj, CommandInfo* cmd)
{
    Fighter* fp = GET_FIGHTER(gobj);
    fp->x221E_b4 = FT_PACKED_U(cmd,set_article_vis.value,0,0x3ffffffU);
    NEXT_CMD(cmd);
}

void ftAction_80071FA0(Fighter_GObj* gobj, CommandInfo* cmd)
{
    Fighter* fp = GET_FIGHTER(gobj);
    fp->x221E_b5 = FT_PACKED_U(cmd,set_fighter_vis.value,0,0x3ffffffU);
    NEXT_CMD(cmd);
}

void ftAction_80071FC8(Fighter_GObj* gobj, CommandInfo* cmd)
{
    Fighter* fp;
    s32 rand;
    u8 volume;
    u8 panning;
    u32 behavior;
    s32 sfx_id;

    fp = GET_FIGHTER(gobj);
    volume = FT_PACKED_U(cmd,pseudo_random_sfx_0.volume,18,0xffU);
    panning = FT_PACKED_U(cmd,pseudo_random_sfx_0.panning,10,0xffU);
    behavior = FT_PACKED_U(cmd,pseudo_random_sfx_0.behavior,6,0xfU);
    rand = FT_PACKED_U(cmd,pseudo_random_sfx_0.random_range,0,0x3fU);
    NEXT_CMD(cmd);
    rand = HSD_Randi(rand);
    switch (rand) {
    case 0:
        sfx_id = FT_PACKED_U(cmd,pseudo_random_sfx_1.sfx_id,0,0xffffffffU);
        NEXT_CMD(cmd);
        NEXT_CMD(cmd);
        NEXT_CMD(cmd);
        NEXT_CMD(cmd);
        NEXT_CMD(cmd);
        break;
    case 1:
        NEXT_CMD(cmd);
        sfx_id = FT_PACKED_U(cmd,pseudo_random_sfx_1.sfx_id,0,0xffffffffU);
        NEXT_CMD(cmd);
        NEXT_CMD(cmd);
        NEXT_CMD(cmd);
        NEXT_CMD(cmd);
        break;
    case 2:
        NEXT_CMD(cmd);
        NEXT_CMD(cmd);
        sfx_id = FT_PACKED_U(cmd,pseudo_random_sfx_1.sfx_id,0,0xffffffffU);
        NEXT_CMD(cmd);
        NEXT_CMD(cmd);
        NEXT_CMD(cmd);
        break;
    case 3:
        NEXT_CMD(cmd);
        NEXT_CMD(cmd);
        NEXT_CMD(cmd);
        sfx_id = FT_PACKED_U(cmd,pseudo_random_sfx_1.sfx_id,0,0xffffffffU);
        NEXT_CMD(cmd);
        NEXT_CMD(cmd);
        break;
    case 4:
        NEXT_CMD(cmd);
        NEXT_CMD(cmd);
        NEXT_CMD(cmd);
        NEXT_CMD(cmd);
        sfx_id = FT_PACKED_U(cmd,pseudo_random_sfx_1.sfx_id,0,0xffffffffU);
        NEXT_CMD(cmd);
        break;
    case 5:
        NEXT_CMD(cmd);
        NEXT_CMD(cmd);
        NEXT_CMD(cmd);
        NEXT_CMD(cmd);
        NEXT_CMD(cmd);
        sfx_id = FT_PACKED_U(cmd,pseudo_random_sfx_1.sfx_id,0,0xffffffffU);
        break;
    }
    NEXT_CMD(cmd);
    switch (behavior) {
    case 0:
        ft_PlaySFX(fp, sfx_id, volume, panning);
        return;
    case 1:
        ft_80088478(fp, sfx_id, volume, panning);
        return;
    case 2:
        ft_800881D8(fp, sfx_id, volume, panning);
        return;
    case 3:
        ft_80088510(fp, sfx_id, volume, panning);
        return;
    case 4:
        ft_800885A8(fp, sfx_id, volume, panning);
        return;
    case 5:
        ft_80088640(fp, sfx_id, volume, panning);
        return;
    case 6:
        ft_80088328(fp, sfx_id, volume, panning);
    default:
        return;
    }
}

void ftAction_800722C8(Fighter_GObj* gobj, CommandInfo* cmd)
{
    SKIP_CMD(cmd, 7);
}

void ftAction_80072320(Fighter_GObj* gobj, CommandInfo* cmd)
{
    Fighter* fp;
    u8 pitch_select;
    s32 spC;
    s32 sp8;
    s32 sfx;
    u32 behavior;
    u32 sfx_base;
    f32 direction;
    s32 sfx_param0;
    s32 sfx_param1;
    s32 sfx_param2;

    fp = GET_FIGHTER(gobj);
    pitch_select = FT_PACKED_U(cmd,stage_sfx_0.pitch_select,0,0xffU);
    behavior = FT_PACKED_U(cmd,stage_sfx_0.sfx_base,16,0x3ffU);
    sfx_base = FT_PACKED_U(cmd,stage_sfx_0.x2_b0_7,8,0xffU);

    switch (pitch_select) {
    case 0:
        direction = 0.0f;
        break;
    case 1:
        direction = 1.0f;
        break;
    case 2:
        direction = -1.0f;
        break;
    case 3:
        direction = fp->facing_dir;
        break;
    }

    NEXT_CMD(cmd);
    sfx = ft_80087D0C(fp, FT_PACKED_U(cmd,stage_sfx_1.sfx_id,0,0xffffffffU));
    NEXT_CMD(cmd);
    sfx_param2 = FT_PACKED_U(cmd,stage_sfx_2.x2_b0_15,0,0xffffU);
    NEXT_CMD(cmd);
    sfx_param0 = FT_PACKED_U(cmd,stage_sfx_3.x2_b0_7,8,0xffU);
    sfx_param1 = FT_PACKED_U(cmd,stage_sfx_3.x3_b0_7,0,0xffU);
    NEXT_CMD(cmd);

    switch (sfx_base) {
    case 0:
        sp8 = 0;
        spC = -1;
        fp->x2160 = lbAudioAx_800264E4(
            lbAudioAx_800263E8(direction, gobj, behavior, sfx, 127, 127,
                               sfx_param0, sfx_param1, sfx_param2, sp8, spC));
        break;

    case 1:
        sp8 = fp->player_idx + fp->is_sub_fighter;
        fp->x214C = lbAudioAx_800264E4(lbAudioAx_800263E8(
            direction, gobj, behavior, sfx, 127, 127, sfx_param0, sfx_param1,
            sfx_param2, sp8 + 0x36, -1));
        break;

    case 2:
        if (!fp->x2225_b6) {
            sp8 = fp->player_idx + fp->is_sub_fighter;
            fp->x2144 = lbAudioAx_800264E4(lbAudioAx_800263E8(
                direction, gobj, behavior, sfx, 127, 127, sfx_param0,
                sfx_param1, sfx_param2, sp8 + 0x1E, -1));
            break;
        }
        switch (fp->kind) {
        case Ft_Kind_GameWatch:
        case Ft_Kind_Samus:
            sp8 = fp->player_idx + fp->is_sub_fighter;
            fp->x2144 = lbAudioAx_800264E4(lbAudioAx_800263E8(
                direction, gobj, behavior, sfx, 127, 127, sfx_param0,
                sfx_param1, sfx_param2, sp8 + 0x1E, -1));
            break;
        default:
            break;
        }
        break;

    case 3:
        sp8 = fp->player_idx + fp->is_sub_fighter;
        fp->x2150 = lbAudioAx_800264E4(lbAudioAx_800263E8(
            direction, gobj, behavior, sfx, 127, 127, sfx_param0, sfx_param1,
            sfx_param2, sp8 + 0x42, -1));
        break;

    case 4:
        sp8 = fp->player_idx + fp->is_sub_fighter;
        fp->x2154 = lbAudioAx_800264E4(lbAudioAx_800263E8(
            direction, gobj, behavior, sfx, 127, 127, sfx_param0, sfx_param1,
            sfx_param2, sp8 + 0x4E, -1));
        break;

    case 5:
        sp8 = fp->player_idx + fp->is_sub_fighter;
        fp->x2158 = lbAudioAx_800264E4(lbAudioAx_800263E8(
            direction, gobj, behavior, sfx, 127, 127, sfx_param0, sfx_param1,
            sfx_param2, sp8 + 0x5A, -1));
        break;

    case 6:
        if (!fp->x2225_b6) {
            sp8 = fp->player_idx + fp->is_sub_fighter;
            fp->x2148 = lbAudioAx_800264E4(lbAudioAx_800263E8(
                direction, gobj, behavior, sfx, 127, 127, sfx_param0,
                sfx_param1, sfx_param2, sp8 + 0x2A, -1));
            break;
        default:
            break;
        }

        switch (fp->kind) {
        case Ft_Kind_GameWatch:
        case Ft_Kind_Samus:
            sp8 = fp->player_idx + fp->is_sub_fighter;
            fp->x2148 = lbAudioAx_800264E4(lbAudioAx_800263E8(
                direction, gobj, behavior, sfx, 127, 127, sfx_param0,
                sfx_param1, sfx_param2, sp8 + 0x2A, -1));
            break;
        default:
            break;
        }
        break;
    }
}

void ftAction_800726C0(Fighter_GObj* gobj, CommandInfo* cmd)
{
    SKIP_CMD(cmd, 4);
}

void ftAction_800726F4(Fighter_GObj* gobj, CommandInfo* cmd)
{
    ftAnim_800704F0(gobj, FT_PACKED_S(cmd,set_tex_anim.idx,18,7),
                    FT_PACKED_S(cmd,set_tex_anim.frame,0,11));
    if (FT_PACKED_U(cmd,set_tex_anim.b,25,0x1U)) {
        ftAnim_800704F0(gobj, FT_PACKED_S(cmd,set_tex_anim.idx2,11,7),
                        FT_PACKED_S(cmd,set_tex_anim.frame,0,11));
    }
    NEXT_CMD(cmd);
}

void ftAction_800727C8(Fighter_GObj* gobj, CommandInfo* cmd)
{
    ftAnim_ApplyPartAnim(gobj, FT_PACKED_S(cmd,part_anim.unk1,19,7),
                         FT_PACKED_S(cmd,part_anim.unk2,12,7), FT_PACKED_U(cmd,part_anim.unk3,0,0xfffU));
    NEXT_CMD(cmd);
}

void ftAction_8007283C(Fighter_GObj* gobj, CommandInfo* cmd)
{
    ftAnim_ApplyPartAnim(gobj, FT_PACKED_S(cmd,part_anim.unk1,19,7),
                         FT_PACKED_S(cmd,part_anim.unk2,12,7), 0.0f);
    NEXT_CMD(cmd);
}

void ftAction_80072894(Fighter_GObj* gobj, CommandInfo* cmd)
{
    ftCommon_8007E83C(gobj, FT_PACKED_U(cmd,unk9.unk1,13,0x1fffU), FT_PACKED_U(cmd,unk9.unk2,0,0x1fffU));
    NEXT_CMD(cmd);
}

void ftAction_800728F8(Fighter_GObj* gobj, CommandInfo* cmd)
{
    Fighter* fp = GET_FIGHTER(gobj);

    if (FT_PACKED_U(cmd,unk10.unk1,25,0x1U)) {
        ftCommon_8007EC30(FT_PACKED_U(cmd,unk10.unk2,13,0xfffU), FT_PACKED_U(cmd,unk10.unk3,0,0x1fffU));
    } else {
        ftCommon_8007EBAC(fp, FT_PACKED_U(cmd,unk10.unk2,13,0xfffU), FT_PACKED_U(cmd,unk10.unk3,0,0x1fffU));
    }
    NEXT_CMD(cmd);
}

void ftAction_8007296C(Fighter_GObj* gobj, CommandInfo* cmd)
{
    NEXT_CMD(cmd);
}

void ftAction_8007297C(Fighter_GObj* gobj, CommandInfo* cmd)
{
    Fighter* fp = GET_FIGHTER(gobj);
    ftCommon_8007ECD4(fp, FT_PACKED_U(cmd,unk11.unk1,0,0x3ffffffU));
    NEXT_CMD(cmd);
}

void ftAction_800729C4(Fighter_GObj* gobj, CommandInfo* cmd)
{
    NEXT_CMD(cmd);
}

void ftAction_800729D4(Fighter_GObj* gobj, CommandInfo* cmd)
{
    Fighter* fp = GET_FIGHTER(gobj);
    int unk2 = FT_PACKED_U(cmd,unk12.unk2,14,0x3ffU);
    int unk3 = FT_PACKED_U(cmd,unk12.unk3,0,0x3fffU);
    switch (FT_PACKED_U(cmd,unk12.unk1,24,0x3U)) {
    case 0:
        ftCommon_8007EEC8(fp, unk2, unk3);
        break;
    case 1:
        ftCommon_8007EF5C(fp, unk2);
        break;
    }
    NEXT_CMD(cmd);
}

void ftAction_80072A4C(Fighter_GObj* gobj, CommandInfo* cmd)
{
    NEXT_CMD(cmd);
}

void ftAction_80072A5C(Fighter_GObj* gobj, CommandInfo* cmd)
{
    Fighter* fp = GET_FIGHTER(gobj);
    ftCo_800BFFD0(fp, FT_PACKED_U(cmd,unk13.unk1,18,0xffU), FT_PACKED_U(cmd,unk13.unk2,0,0x3ffffU));
    NEXT_CMD(cmd);
}

void ftAction_80072AAC(Fighter_GObj* gobj, CommandInfo* cmd)
{
    NEXT_CMD(cmd);
}

void ftAction_80072ABC(Fighter_GObj* gobj, CommandInfo* cmd)
{
    ftCo_800C0200(GET_FIGHTER(gobj), FT_PACKED_U(cmd,unk14.unk1,18,0xffU));
    NEXT_CMD(cmd);
}

void ftAction_80072B04(Fighter_GObj* gobj, CommandInfo* cmd)
{
    NEXT_CMD(cmd);
}

void ftAction_80072B14(Fighter_GObj* gobj, CommandInfo* cmd)
{
    Fighter* fp = GET_FIGHTER(gobj);
    fp->x221E_b1 = FT_PACKED_U(cmd,unk15.unk1,0,0x3ffffffU);
    NEXT_CMD(cmd);
}

void ftAction_80072B3C(Fighter_GObj* gobj, CommandInfo* cmd)
{
    Fighter* fp = GET_FIGHTER(gobj);
    fp->x2100 = FT_PACKED_S(cmd,unk16.unk4,0,25);
    fp->x2101_bits_8 = FT_PACKED_S(cmd,unk16.unk3,25,1);
    NEXT_CMD(cmd);
}

void ftAction_80072B84(Fighter_GObj* gobj, CommandInfo* cmd)
{
    NEXT_CMD(cmd);
}

void ftAction_80072B94(Fighter_GObj* gobj, CommandInfo* cmd)
{
    Fighter* fp = GET_FIGHTER(gobj);
    ftCo_8009E318(gobj, FT_PACKED_S(cmd,unk17.unk1,0,26), fp->cur_anim_frame);
    NEXT_CMD(cmd);
}

void ftAction_80072BE4(Fighter_GObj* gobj, CommandInfo* cmd)
{
    NEXT_CMD(cmd);
}

void ftAction_80072BF4(Fighter_GObj* gobj, CommandInfo* cmd)
{
    Fighter* fp = GET_FIGHTER(gobj);
    Fighter_TakeDamage_8006CC7C(fp, FT_PACKED_S(cmd,unk18.damage_amount,0,26));
    NEXT_CMD(cmd);
}

void ftAction_80072C5C(Fighter_GObj* gobj, CommandInfo* cmd)
{
    NEXT_CMD(cmd);
}

void ftAction_80072C6C(Fighter_GObj* gobj, CommandInfo* cmd)
{
    ft_8008A1B8(gobj, FT_PACKED_U(cmd,unk19.unk1,0,0x3ffffffU));
    NEXT_CMD(cmd);
}

void ftAction_80072CB0(Fighter_GObj* gobj, CommandInfo* cmd)
{
    Fighter* fp = GET_FIGHTER(gobj);
    fp->x2225_b2 = FT_PACKED_U(cmd,unk20.unk1,0,0x3ffffffU);
    NEXT_CMD(cmd);
}

void ftAction_80072CD8(Fighter_GObj* gobj, CommandInfo* cmd)
{
    int sp64;
    int sp60;
    int gfx_id;
    CommandInfo _cmd;
    u32 cmd_words[3];
    Vec3 offset;
    Vec3 range;
    u32 part;
    Fighter* fp;
    u32 unused;

    fp = gobj->user_data;
    sp60 = 1;

    if (ft_80084BFC(gobj, &sp64, &sp60, &gfx_id) != false) {
        if (sp64 != -1) {
            _cmd.x8.u = (union CmdUnion*) cmd_words;
            cmd_words[0] = FT_PACKED_WORD(cmd, 0);
            cmd_words[1] = sp64;
            cmd_words[2] = FT_PACKED_WORD(cmd, 8);
            ftAction_80071B50(gobj, &_cmd);
        }

        if (gfx_id != -1) {
            offset.z = 0.0f;
            range.z = 0.0f;
            offset.y = 0.0f;
            range.y = 0.0f;
            offset.x = 0.0f;
            range.x = 0.0f;

            if (!FT_PACKED_U(cmd,footstep_fx_0.use_alt_bone,17,1U)) {
                part = fp->ft_data->x8->x13;
            } else {
                part = fp->ft_data->x8->x14;
            }
            ftCo_8009F834(gobj, gfx_id, part, 0, 0, &offset, &range, 0.0f);
        }
    }

    if (sp60 != 0) {
        ftAction_SoundPacked(gobj, cmd);
        return;
    }

    ++cmd->x8.u;
    ++cmd->x8.u;
    ++cmd->x8.u;
}

void ftAction_80072E24(Fighter_GObj* gobj, CommandInfo* cmd)
{
    SKIP_CMD(cmd, 3);
}

void ftAction_80072E4C(Fighter_GObj* gobj, CommandInfo* cmd)
{
    int sp64;
    int sp60;
    int gfx_id;
    CommandInfo _cmd;
    u32 cmd_words[3];
    Vec3 offset;
    Vec3 range;
    Fighter* fp;
    s32 cmd_flag;
    u32 unused;

    fp = gobj->user_data;
    sp60 = 1;
    gfx_id = -1;
    cmd_flag = FT_PACKED_U(cmd,unk_fx_0.x1_b0_7,16,255U);
    cmd_flag &= 1;
    if ((ft_80084C38(gobj, &sp64, &sp60, &gfx_id) != false) &&
        (cmd_flag == 0) && (sp64 != -1))
    {
        _cmd.x8.u = (union CmdUnion*) cmd_words;
        cmd_words[0] = FT_PACKED_WORD(cmd, 0);
        cmd_words[1] = sp64;
        cmd_words[2] = FT_PACKED_WORD(cmd, 8);
        ftAction_80071B50(gobj, &_cmd);
    }

    if (gfx_id == -1) {
        gfx_id = FT_PACKED_HALF(cmd, 2);
    }
    offset.z = 0.0f;
    range.z = 0.0f;
    offset.y = 0.0f;
    range.y = 0.0f;
    offset.x = 0.0f;
    range.x = 0.0f;
    ftCo_8009F834(gobj, gfx_id, FtPart_TopN, 0, 0, &offset, &range, 0.0f);
    if (sp60 != 0) {
        ft_PlaySFX(fp, 0x46, 0x7FU, 0x40U);
        if (cmd_flag == 0) {
            ftAction_SoundPacked(gobj, cmd);
        }
    }
    if ((sp60 == 0) || (cmd_flag != 0)) {
        ++cmd->x8.u;
        ++cmd->x8.u;
        ++cmd->x8.u;
    }
    ftCommon_8007EBAC(fp, 0x16U, 0U);
}

void ftAction_80072FE0(Fighter_GObj* gobj, CommandInfo* cmd)
{
    SKIP_CMD(cmd, 3);
}

void ftAction_80073008(Fighter_GObj* gobj, CommandInfo* cmd)
{
    f32 charge_frames;
    f32 charge_rate;
    f32 color_anim;
    f32 dmg_mult;

    charge_rate = FT_PACKED_U(cmd,smash_charge_0.charge_rate,0,0xffffU);
    charge_frames = FT_PACKED_U(cmd,smash_charge_0.charge_frames,16,0x3ffU);
    dmg_mult = 0.003906f * charge_rate;
    NEXT_CMD(cmd);
    color_anim = FT_PACKED_U(cmd,smash_charge_1.color_anim,24,0xffU);
    NEXT_CMD(cmd);
    ftCo_800DEE84(gobj, (s32) color_anim, charge_frames, dmg_mult);
}

void ftAction_8007309C(Fighter_GObj* gobj, CommandInfo* cmd)
{
    SKIP_CMD(cmd, 2);
}

void ftAction_800730B8(Fighter_GObj* gobj, CommandInfo* cmd)
{
    Fighter* fp = GET_FIGHTER(gobj);
    ftCo_800C8B60(fp, FT_PACKED_U(cmd,unk21.unk1,25,0x1U), FT_PACKED_U(cmd,unk21.unk2,17,0xffU));
    NEXT_CMD(cmd);
}

void ftAction_80073108(Fighter_GObj* gobj, CommandInfo* cmd)
{
    NEXT_CMD(cmd);
}

void ftAction_80073118(Fighter_GObj* gobj, CommandInfo* cmd)
{
    u8 idx;
    s32 timer;
    f32 x;
    f32 y;
    f32 mag;
    f32 decay_amt;
    f32 angle;

    idx = FT_PACKED_U(cmd,wind_fx_0.bone,0,0xffU);
    NEXT_CMD(cmd);

    x = 0.003906f * FT_PACKED_S(cmd,wind_fx_1.timer,16,16);
    y = 0.003906f * FT_PACKED_S(cmd,wind_fx_1.x,0,16);
    NEXT_CMD(cmd);

    mag = 0.003906f * FT_PACKED_S(cmd,wind_fx_2.y,16,16);
    decay_amt = 0.003906f * FT_PACKED_S(cmd,wind_fx_2.mag,0,16);
    NEXT_CMD(cmd);

    timer = FT_PACKED_S(cmd,wind_fx_3.angle,16,16);
    angle = 0.003906f * FT_PACKED_S(cmd,wind_fx_3.decay,0,16);
    NEXT_CMD(cmd);

    ftCo_8009E714(gobj, idx, timer, x, y, mag, decay_amt, angle);
}

void ftAction_8007320C(Fighter_GObj* gobj, CommandInfo* cmd)
{
    SKIP_CMD(cmd, 4);
}

/// @todo Fix naming.
#define gmScriptEventCast(p_event, type) ((type*) (p_event))

void ftAction_80073240(Fighter_GObj* fighter_gobj)
{
    Fighter* fp = fighter_gobj->user_data;
    CommandInfo* ftCommand = &fp->x3E4_fighterCmdScript;
    u32 eventCode;
    fp->x3E4_fighterCmdScript.frame_count = fp->cur_anim_frame + fp->x898_unk;
    if (fp->x3E4_fighterCmdScript.x8.u != NULL) {
        if (ftCommand->timer != F32_MAX) {
            ftCommand->timer -= fp->frame_speed_mul;
        }
        do {
            if (ftCommand->x8.u == NULL) {
                break;
            }
            if (F32_MAX == ftCommand->timer) {
                if (ftCommand->frame_count >= fp->frame_speed_mul) {
                    break;
                }
                ftCommand->timer = -ftCommand->frame_count;
            } else if (ftCommand->timer > 0.0f) {
                break;
            }
            eventCode = FT_FIGHTER_OPCODE(ftCommand);
            if (FT_FIGHTER_EXECUTE(ftCommand, eventCode) == false) {
#ifdef TARGET_PC
                HSD_ASSERT(0, eventCode < ARRAY_SIZE(ftAction_803C06E8) + 10);
#endif
                eventCode -= 0xA;
                ftAction_803C06E8[eventCode](fighter_gobj, ftCommand);
            }
        } while (F32_MAX != ftCommand->timer);
    }
}

void ftAction_80073354(Fighter_GObj* gobj)
{
    Fighter* fp = gobj->user_data;
    CommandInfo* cmd = (&fp->x3E4_fighterCmdScript);
    u32 eventCode;

    fp->x3E4_fighterCmdScript.frame_count = fp->cur_anim_frame + fp->x898_unk;
    fp->x2210.throw_flags = 0;
    if (fp->x3E4_fighterCmdScript.x8.u != NULL) {
        if (cmd->timer != F32_MAX) {
            cmd->timer -= fp->frame_speed_mul;
        }
        do {
            if (cmd->x8.u == NULL) {
                break;
            }
            if (cmd->timer == F32_MAX) {
                if (cmd->frame_count >= fp->frame_speed_mul) {
                    break;
                }
                cmd->timer = -cmd->frame_count;
            } else if (cmd->timer > 0.0f) {
                break;
            }
            {
                float timer = cmd->timer;
                eventCode = FT_FIGHTER_OPCODE(cmd);
                if (FT_FIGHTER_EXECUTE(cmd, eventCode) == false) {
#ifdef TARGET_PC
                    HSD_ASSERT(0, eventCode < ARRAY_SIZE(ftAction_803C07AC) + 10);
#endif
                    eventCode -= 0xA;
                    ftAction_803C07AC[eventCode](gobj, cmd);
                }
                if (cmd->timer != timer && cmd->timer <= 0.0f) {
                    fp->x2210.throw_flags = 0;
                }
            }
        } while (cmd->timer != F32_MAX);
    }
}

void ftAction_8007349C(Fighter_GObj* gobj)
{
    Fighter* fp = GET_FIGHTER(gobj);
    CommandInfo* cmd = (&fp->x3E4_fighterCmdScript);
    PAD_STACK(8);
    fp->x3E4_fighterCmdScript.frame_count = fp->cur_anim_frame + fp->x898_unk;
    if (fp->x3E4_fighterCmdScript.x8.u == NULL) {
        return;
    }
    if (cmd->timer != F32_MAX) {
        cmd->timer -= fp->frame_speed_mul;
    }
    do {
        if (cmd->x8.u == NULL) {
            break;
        }
        if (cmd->timer == F32_MAX) {
            if (cmd->frame_count >= fp->frame_speed_mul) {
                break;
            }
            cmd->timer = -cmd->frame_count;
        } else if (cmd->timer > 0.0f) {
            break;
        }
        {
            u32 id = FT_FIGHTER_OPCODE(cmd);
            if (!FT_FIGHTER_EXECUTE(cmd, id)) {
#ifdef TARGET_PC
                HSD_ASSERT(0, id < ARRAY_SIZE(ftAction_803C0870) + 10);
#endif
                cmd->x8.u += ftAction_803C0870[id - 10];
            }
        }
    } while (cmd->timer != F32_MAX);
}
