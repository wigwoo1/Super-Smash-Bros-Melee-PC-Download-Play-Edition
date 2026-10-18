#include "itluigifireball.h"

#include <Runtime/platform.h>
#ifdef TARGET_PC
#include <stdlib.h>
#include <dolphin/os.h>
#endif

#include "inlines.h"
#include <dolphin/mtx.h>
#include <melee/db/db.h>
#include <melee/ef/efasync.h>
#include <melee/it/inlines.h>
#include <melee/it/it_26B1.h>
#include <melee/it/it_2725.h>
#include <melee/it/itCommonItems.h>
#include <melee/it/item.h>
#include <melee/it/itgroundcoll.h>
#include <melee/it/itmaplib.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/mtx.h>

/* 2C027C */ static void it_802C027C(Item_GObj* gobj);
/* 2C02E4 */ static bool itLuigifireball_UnkMotion0_Anim(Item_GObj* gobj);
/* 2C031C */ static void itLuigifireball_UnkMotion0_Phys(Item_GObj* gobj);
/* 2C0368 */ static bool itLuigifireball_UnkMotion0_Coll(Item_GObj* gobj);

ItemStateTable it_803F75C0[] = {
    { 0, itLuigifireball_UnkMotion0_Anim, itLuigifireball_UnkMotion0_Phys,
      itLuigifireball_UnkMotion0_Coll },
};

void it_802C01AC(Item_GObj* gobj, Vec3* pos, ItemKind kind, float facing_dir)
{
    SpawnItem spawn_item;
    spawn_item.kind = kind;
    spawn_item.prev_pos = *pos;
    spawn_item.prev_pos.z = 0;
    it_8026BB68(gobj, &spawn_item.pos);
    Item_InitSpawnCommonFields(&spawn_item, gobj, facing_dir, true);
    {
        u8 _[8];
        Item_GObj* spawned_gobj = Item_80268B18(&spawn_item);
        if (spawned_gobj != NULL) {
            it_802C027C(spawned_gobj);
            db_80225DD8(spawned_gobj, gobj);
            it_802750F8(spawned_gobj);
#ifdef TARGET_PC
            if (kind == It_Kind_Luigi_Fire &&
                getenv("MELEE_DIAGNOSTIC_MATCH_SPECIALS")) {
                Item* observed = GET_ITEM(spawned_gobj);
                OSReport("NATIVE_LUIGI_FIREBALL initialized=1 kind=%d x=%f y=%f vx=%f vy=%f life=%f\n",
                         observed->kind, observed->pos.x, observed->pos.y,
                         observed->x40_vel.x, observed->x40_vel.y,
                         observed->xD44_lifeTimer);
            }
            if (kind == It_Kind_Kirby_LuigiFire &&
                getenv("MELEE_DIAGNOSTIC_KIRBY_LUIGI_COPY_INPUT"))
                OSReport("NATIVE_KIRBY_LUIGI_FIREBALL spawned=1 kind=%d x=%f y=%f facing=%f\n",
                         kind, pos->x, pos->y, facing_dir);
#endif
        }
    }
}

void it_802C027C(Item_GObj* gobj)
{
    Item* ip = GET_ITEM(gobj);
    itUnkAttributes* attrs = ip->xC4_article_data->x4_specialAttributes;
    ip->x40_vel.x = attrs->x0_float * ip->facing_dir;
    ip->x40_vel.y = ip->x40_vel.z = 0;
    it_80275158(gobj, attrs->x4_float);
    Item_80268E5C(gobj, 0, ITEM_ANIM_UPDATE);
}

bool itLuigifireball_UnkMotion0_Anim(Item_GObj* gobj)
{
    Item* item = GET_ITEM(gobj);
    --item->xD44_lifeTimer;
#ifdef TARGET_PC
    if (item->kind == It_Kind_Luigi_Fire &&
        getenv("MELEE_DIAGNOSTIC_LUIGI_FIREBALL_TRAVEL")) {
        static unsigned samples;
        if (samples < 16) {
            ++samples;
            OSReport("NATIVE_LUIGI_FIREBALL_TRAVEL sample=%u id=%p kind=%d x=%f y=%f vx=%f vy=%f life=%f\n",
                     samples, (void*)gobj, item->kind, item->pos.x,
                     item->pos.y, item->x40_vel.x, item->x40_vel.y,
                     item->xD44_lifeTimer);
        }
    }
#endif
    return item->xD44_lifeTimer <= 0 ? true : false;
}

void itLuigifireball_UnkMotion0_Phys(Item_GObj* gobj)
{
    Item_ApplyFallingPhysics(gobj);
}

static double calc_dist_2d_accurate(Vec3* v)
{
    return sqrtf_accurate(VEC2_SQ_LEN(*v));
}

bool itLuigifireball_UnkMotion0_Coll(Item_GObj* gobj)
{
    it_8026D9A0(gobj);
    if (it_8027781C(gobj)) {
        Item* ip = GET_ITEM(gobj);
        HSD_JObj* jobj = GET_JOBJ(gobj);
        itUnkAttributes* attrs = ip->xC4_article_data->x4_specialAttributes;
        if (calc_dist_2d_accurate(&ip->x40_vel) < attrs->xC) {
            return true;
        }
        if (ip->kind == It_Kind_Luigi_Fire) {
            efAsync_Spawn(gobj, &(GET_ITEM(gobj)->xBC0), 1, 1288, jobj);
        } else {
            efAsync_Spawn(gobj, &(GET_ITEM(gobj)->xBC0), 1, 1202, jobj);
        }
    }
    return false;
}

bool itLuigiFireball_Logic89_DmgDealt(Item_GObj* gobj)
{
#ifdef TARGET_PC
    if (GET_ITEM(gobj)->kind == It_Kind_Luigi_Fire &&
        getenv("MELEE_DIAGNOSTIC_LUIGI_FIREBALL_HIT"))
        OSReport("NATIVE_LUIGI_FIREBALL_DAMAGE_CALLBACK kind=%d\n",
                 GET_ITEM(gobj)->kind);
    if (GET_ITEM(gobj)->kind == It_Kind_Kirby_LuigiFire &&
        getenv("MELEE_DIAGNOSTIC_KIRBY_LUIGI_FIREBALL_HIT"))
        OSReport("NATIVE_KIRBY_LUIGI_FIREBALL_DAMAGE_CALLBACK kind=%d\n",
                 GET_ITEM(gobj)->kind);
#endif
    return true;
}

bool itLuigiFireball_Logic89_Reflected(Item_GObj* gobj)
{
    return it_80273030(gobj);
}

bool itLuigiFireball_Logic89_Clanked(Item_GObj* gobj)
{
    return true;
}

bool itLuigiFireball_Logic89_HitShield(Item_GObj* gobj)
{
    return true;
}

bool itLuigiFireball_Logic89_Absorbed(Item_GObj* gobj)
{
    return true;
}

bool itLuigiFireball_Logic89_ShieldBounced(Item_GObj* gobj)
{
    return itColl_BounceOffShield(gobj);
}

void itLuigiFireball_Logic89_EvtUnk(Item_GObj* gobj, HSD_GObj* referenced_gobj)
{
    it_8026B894(gobj, referenced_gobj);
}
