#ifndef MELEE_PC_ADVENTURE_KOOPA_DIAGNOSTIC_H
#define MELEE_PC_ADVENTURE_KOOPA_DIAGNOSTIC_H
#include <melee/gr/types.h>
#include <melee/it/types.h>
#include <melee/ft/fighter.h>
#include <sysdolphin/baselib/aobj.h>
#include <sysdolphin/baselib/jobj.h>
/* Opt-in controller-only grounded-Koopa probe. All actors, damage, stage,
 * stocks and outcomes are read-only. Main stick+A respects 1P input rules. */
static float MeleePc_KoopaAbs(float value){return value<0?-value:value;}
static void MeleePc_AdventureKoopaInput(HSD_GObj* self,PADStatus* input,unsigned frame){
 memset(input,0,sizeof(*input));
 if(!self || gm_GetCurrentGameMode()!=GM_ADVENTURE || Stage_8022519C(Stage_80225194())!=Gr_Kind_KinokoRoute)return;
 Fighter* fp=self->user_data;Item* nearest=NULL;HSD_GObj* target=NULL;
 Item* shell_item=NULL;HSD_GObj* shell_target=NULL;float shell_distance=1.0e30f;
 static int ready,collapse_seen;static unsigned progress_frame,jump_until,attack_start;
 static float progress_x,goal_x;float distance=1.0e30f;unsigned count=0,shells=0;
 if(!ready && frame>=180){
  ready=1;progress_frame=frame;progress_x=fp->cur_pos.x;goal_x=fp->cur_pos.x+200;
  grZakoGenerator_SpawnDesc* spawns=(grZakoGenerator_SpawnDesc*)((u8*)Ground_GetYakumonoParam()+4);
  float best=1.0e30f;
  for(unsigned i=0;i<80;i++)if(spawns[i].kind==It_Kind_Nokonoko){
   Vec3 pos;if(Ground_801C2D24((s32)i+0x20,&pos)){
    OSReport("NATIVE_KOOPA_SPAWN_SOURCE index=%u marker=%u x=%f y=%f kind=%u variant=%u\n",i,i+0x20,pos.x,pos.y,spawns[i].kind,spawns[i].x2);
    float dx=pos.x-fp->cur_pos.x;if(dx>20 && dx<best){best=dx;goal_x=pos.x;}
   }
  }
  OSReport("NATIVE_KOOPA_ROUTE_BEGIN frame=%u x=%f y=%f goal_x=%f\n",frame,fp->cur_pos.x,fp->cur_pos.y,goal_x);
 }
 if(!ready)return;
 for(HSD_GObj* g=HSD_GObjPLinkHead[HSD_GOBJ_PLINK_ITEM];g;g=g->next){
  Item* item=g->user_data;if(!item)continue;
  if(item->kind==It_Kind_ZGShell || item->kind==It_Kind_ZRShell){
   shells++;float d=MeleePc_KoopaAbs(item->pos.x-fp->cur_pos.x)+MeleePc_KoopaAbs(item->pos.y-fp->cur_pos.y);
   if(d<shell_distance){shell_distance=d;shell_item=item;shell_target=g;}continue;
  }
  if(item->kind!=It_Kind_Nokonoko)continue;
  count++;if(item->msid==5)collapse_seen=1;
  float d=MeleePc_KoopaAbs(item->pos.x-fp->cur_pos.x)+MeleePc_KoopaAbs(item->pos.y-fp->cur_pos.y);
  if(d<distance){distance=d;nearest=item;target=g;}
 }
 if(shells)collapse_seen=1;
 if(!collapse_seen && frame<2100 && Player_GetStocks(0)>0){
  float dx=(nearest?nearest->pos.x:goal_x)-fp->cur_pos.x;
  float dy=nearest?nearest->pos.y-fp->cur_pos.y:0;int sign=dx>=0?1:-1;
  if(!nearest || MeleePc_KoopaAbs(dx)>25 || MeleePc_KoopaAbs(dy)>20){
   input->stickX=(s8)(80*sign);
   if(fp->ground_or_air!=GA_Ground){progress_frame=frame;progress_x=fp->cur_pos.x;}
   else if(frame-progress_frame>=45){
    /* Hold a normal jump long enough to clear taller original obstacles. */
    if(MeleePc_KoopaAbs(fp->cur_pos.x-progress_x)<6)jump_until=frame+20;
    progress_frame=frame;progress_x=fp->cur_pos.x;
   }
   if(frame<jump_until)input->button=PAD_BUTTON_X;
   attack_start=0;
  }else if(fp->facing_dir*sign<0){input->stickX=(s8)(35*sign);attack_start=0;}
  else{
   if(!attack_start)attack_start=frame;
   if((frame-attack_start)%45<4){input->stickX=(s8)(80*sign);input->button=PAD_BUTTON_A;}
  }
 }
 if(getenv("MELEE_DIAGNOSTIC_SHELL_INTERACTION") && collapse_seen && frame<2100 && Player_GetStocks(0)>0){
  static unsigned phase,pickup_start,held_frame,release_frame;static HSD_GObj* held_shell;
  Item* held=fp->item_gobj?fp->item_gobj->user_data:NULL;
  int held_is_shell=held && (held->kind==It_Kind_ZGShell || held->kind==It_Kind_ZRShell);
  memset(input,0,sizeof(*input));
  if(held_is_shell && phase<3){
   if(phase<2){
    phase=2;held_frame=frame;held_shell=fp->item_gobj;
    OSReport("NATIVE_SHELL_HELD frame=%u shell=%p owner=%p self=%p owner_player=%d kind=%d fighter_motion=%d shell_motion=%d\n",frame,held_shell,held->owner,self,held->owner && ftLib_IsFighter(held->owner)?ftLib_GetPlayerIndex(held->owner):-1,held->kind,fp->motion_id,held->msid);
   }
   if(frame>=held_frame+40){phase=3;input->stickX=80;input->button=PAD_BUTTON_A;}
  }else if(phase==3){
   if(!held_is_shell){phase=4;release_frame=frame;OSReport("NATIVE_SHELL_RELEASED frame=%u shell=%p fighter_motion=%d\n",frame,held_shell,fp->motion_id);}
   else if(frame<held_frame+44){input->stickX=80;input->button=PAD_BUTTON_A;}
  }else if(phase<2 && shell_item && fp->ground_or_air==GA_Ground){
   float dx=shell_item->pos.x-fp->cur_pos.x;
   if(MeleePc_KoopaAbs(dx)>8){input->stickX=(s8)(dx>0?40:-40);pickup_start=0;}
   else{
    if(!pickup_start)pickup_start=frame;
    phase=1;if((frame-pickup_start)%30<4)input->button=PAD_BUTTON_A;
   }
  }
  /* Ask for normal movement after release; prove control recovery rather
   * than accepting only the item leaving the fighter's hand. */
  if(phase==4 && frame>=release_frame+60 && frame<release_frame+180)input->stickX=40;
  if(phase>=3 && frame%30==0 && frame<1000){
   unsigned active=0,shown=0;unsigned parts=ftPartsTable[fp->kind]->parts_num;
   for(unsigned i=0;i<parts && i<140;i++){
    if(!fp->parts[i].x8.x0.flags_b1 || fp->parts[i].x8.x0.flags_b0 || fp->parts[i].x8.x0.flags_b5)continue;
    HSD_JObj* j=fp->x8A4_animBlendFrames==0?fp->parts[i].joint:fp->parts[i].x4_jobj2;
    HSD_AObj* a=j?j->aobj:NULL;if(!a || (a->flags&AOBJ_NO_ANIM))continue;
    active++;if(shown++<8)
     OSReport("NATIVE_SHELL_ACTIVE_ANIM frame=%u part=%u flags=%08x current=%f end=%f rate=%f fighter_frame=%f fighter_rate=%f blend=%f\n",frame,i,a->flags,a->curr_frame,a->end_frame,a->framerate,fp->cur_anim_frame,fp->frame_speed_mul,fp->x8A4_animBlendFrames);
   }
   OSReport("NATIVE_SHELL_RECOVERY frame=%u motion=%d fighter_frame=%f fighter_rate=%f active_parts=%u input_x=%d x=%f y=%f\n",frame,fp->motion_id,fp->cur_anim_frame,fp->frame_speed_mul,active,input->stickX,fp->cur_pos.x,fp->cur_pos.y);
  }
  if(frame%5==0)
   OSReport("NATIVE_SHELL_ROUTE frame=%u phase=%u self_x=%f self_y=%f motion=%d ground=%d held=%p held_kind=%d shell=%p shell_x=%f shell_y=%f shell_motion=%d shell_vx=%f shell_vy=%f requested_x=%d requested_button=%u\n",frame,phase,fp->cur_pos.x,fp->cur_pos.y,fp->motion_id,fp->ground_or_air,fp->item_gobj,held?held->kind:-1,shell_target,shell_item?shell_item->pos.x:0,shell_item?shell_item->pos.y:0,shell_item?shell_item->msid:-1,shell_item?shell_item->x40_vel.x:0,shell_item?shell_item->x40_vel.y:0,input->stickX,input->button);
 }
 if(frame%15==0)
  OSReport("NATIVE_KOOPA_ROUTE frame=%u self_x=%f self_y=%f motion=%d ground=%d stocks=%d count=%u shells=%u target=%p target_x=%f target_y=%f target_motion=%d target_damage=%d requested_x=%d requested_button=%u collapse_seen=%d\n",frame,fp->cur_pos.x,fp->cur_pos.y,fp->motion_id,fp->ground_or_air,Player_GetStocks(0),count,shells,target,nearest?nearest->pos.x:0,nearest?nearest->pos.y:0,nearest?nearest->msid:-1,nearest?nearest->xC9C:-1,input->stickX,input->button,collapse_seen);
}
#endif
