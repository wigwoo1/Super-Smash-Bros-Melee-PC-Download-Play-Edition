#ifndef MELEE_PC_ADVENTURE_GOOMBA_DIAGNOSTIC_H
#define MELEE_PC_ADVENTURE_GOOMBA_DIAGNOSTIC_H
#include <melee/gr/types.h>
#include <melee/it/types.h>
/* Controller-only Adventure probe. Original Fighter_procInput explicitly
 * suppresses C-stick in single-player modes, so use a fresh main-stick+A
 * pulse for normal ground smash/air attack selection. Actor, stage, damage
 * and collision state remain read-only; retail input rules stay unchanged. */
static float MeleePc_GoombaAbs(float x){return x<0?-x:x;}
static void MeleePc_AdventureGoombaInput(HSD_GObj* self,PADStatus* input,unsigned frame){
 memset(input,0,sizeof(*input));
 if(!self || gm_GetCurrentGameMode()!=GM_ADVENTURE || Stage_8022519C(Stage_80225194())!=Gr_Kind_KinokoRoute)return;
 Fighter* fp=self->user_data;Item* nearest=NULL;HSD_GObj* target=NULL;
 float distance=1.0e30f;unsigned count=0;
 static int ready,damage_seen;static unsigned progress_frame,jump_until,attack_start;
 static float progress_x,goal_x;
 if(!ready && frame>=180){
  ready=1;progress_frame=frame;progress_x=fp->cur_pos.x;goal_x=fp->cur_pos.x+180;
  grZakoGenerator_SpawnDesc* spawns=(grZakoGenerator_SpawnDesc*)((u8*)Ground_GetYakumonoParam()+4);
  float best=1.0e30f;
  for(unsigned i=0;i<80;i++)if(spawns[i].kind==It_Kind_Kuriboh){
   Vec3 pos;if(Ground_801C2D24((s32)i+0x20,&pos)){
    OSReport("NATIVE_GOOMBA_SPAWN_SOURCE index=%u x=%f y=%f kind=%u\n",i,pos.x,pos.y,spawns[i].kind);
    float dx=pos.x-fp->cur_pos.x;if(dx>20 && dx<best){best=dx;goal_x=pos.x;}
   }
  }
  OSReport("NATIVE_GOOMBA_ROUTE_BEGIN frame=%u x=%f y=%f goal_x=%f\n",frame,fp->cur_pos.x,fp->cur_pos.y,goal_x);
 }
 if(!ready)return;
 for(HSD_GObj* g=HSD_GObjPLinkHead[HSD_GOBJ_PLINK_ITEM];g;g=g->next){
  Item* item=g->user_data;if(!item || item->kind!=It_Kind_Kuriboh)continue;
  count++;if(item->xC9C>0)damage_seen=1;
  float d=MeleePc_GoombaAbs(item->pos.x-fp->cur_pos.x)+MeleePc_GoombaAbs(item->pos.y-fp->cur_pos.y);
  if(d<distance){distance=d;nearest=item;target=g;}
 }
 if(!damage_seen && frame<1500 && Player_GetStocks(0)>0){
  float dx=(nearest?nearest->pos.x:goal_x)-fp->cur_pos.x;
  float dy=nearest?nearest->pos.y-fp->cur_pos.y:0;int sign=dx>=0?1:-1;
  if(!nearest || MeleePc_GoombaAbs(dx)>25 || MeleePc_GoombaAbs(dy)>20){
   input->stickX=(s8)(80*sign);
   /* Net progress over 45 frames detects the original wall-collision
    * oscillation; adjacent samples were moving without passing the block. */
   if(fp->ground_or_air!=GA_Ground){progress_frame=frame;progress_x=fp->cur_pos.x;}
   else if(frame-progress_frame>=45){
    if(MeleePc_GoombaAbs(fp->cur_pos.x-progress_x)<6)jump_until=frame+4;
    progress_frame=frame;progress_x=fp->cur_pos.x;
   }
   if(frame<jump_until)input->button=PAD_BUTTON_X;
   attack_start=0;
  }else if(fp->facing_dir*sign<0){input->stickX=(s8)(35*sign);attack_start=0;}
  else {
   if(!attack_start)attack_start=frame;
   if((frame-attack_start)%45<4){
    input->stickX=(s8)(80*sign);input->button=PAD_BUTTON_A;
   }
  }
 }
 if(frame%15==0 || damage_seen){
  static unsigned seen_samples;
  if(!damage_seen || seen_samples++<8)
   OSReport("NATIVE_GOOMBA_ROUTE frame=%u self_x=%f self_y=%f motion=%d ground=%d stocks=%d count=%u target=%p target_x=%f target_y=%f target_motion=%d target_damage=%d requested_x=%d requested_button=%u requested_cx=%d damage_seen=%d\n",
    frame,fp->cur_pos.x,fp->cur_pos.y,fp->motion_id,fp->ground_or_air,Player_GetStocks(0),count,target,
    nearest?nearest->pos.x:0,nearest?nearest->pos.y:0,nearest?nearest->msid:-1,nearest?nearest->xC9C:-1,
    input->stickX,input->button,input->substickX,damage_seen);
 }
}
#endif
