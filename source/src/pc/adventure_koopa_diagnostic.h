#ifndef MELEE_PC_ADVENTURE_KOOPA_DIAGNOSTIC_H
#define MELEE_PC_ADVENTURE_KOOPA_DIAGNOSTIC_H
#include <melee/gr/types.h>
#include <melee/it/types.h>
#include <melee/ft/fighter.h>
#include <melee/mp/mplib.h>
#include <sysdolphin/baselib/aobj.h>
#include <sysdolphin/baselib/jobj.h>
/* Opt-in controller-only grounded-Koopa probe. All actors, damage, stage,
 * stocks and outcomes are read-only. Main stick+A respects 1P input rules. */
static float MeleePc_KoopaAbs(float value){return value<0?-value:value;}
static void MeleePc_AdventureKoopaInput(HSD_GObj* self,PADStatus* input,unsigned frame){
 memset(input,0,sizeof(*input));
 if(!self || gm_GetCurrentGameMode()!=GM_ADVENTURE)return;
 /* Authored maze route: corridor floor19 -> gap -> platform24 -> floor27.
  * Readiness and landing are observed; only ordinary PAD inputs are emitted. */
 if(getenv("MELEE_DIAGNOSTIC_ADVENTURE_MAZE_ROUTE") &&
    Stage_8022519C(Stage_80225194())==Gr_Kind_ShrineRoute){
  Fighter* fp=self->user_data;
  static unsigned jump_frame,gap_landed_frame,raised_jump_frame,raised_second_jump;
  static int route_started,route_done,route_aborted,start_stocks;
  if(!route_started && frame>=180){route_started=1;start_stocks=Player_GetStocks(0);}
  if(route_started && !route_done && !route_aborted){
   if(Player_GetStocks(0)!=start_stocks || fp->cur_pos.y<-100 || frame>1800){
    route_aborted=1;OSReport("NATIVE_MAZE_ROUTE_ABORT frame=%u stocks=%d x=%f y=%f\n",frame,Player_GetStocks(0),fp->cur_pos.x,fp->cur_pos.y);
   }else{
    if(!gap_landed_frame){
    if(fp->cur_pos.x<202)input->stickX=60;
    if(!jump_frame && fp->cur_pos.x>=137 && fp->ground_or_air==GA_Ground){
     jump_frame=frame;OSReport("NATIVE_MAZE_GAP_JUMP frame=%u x=%f y=%f\n",frame,fp->cur_pos.x,fp->cur_pos.y);
    }
    if(jump_frame && frame-jump_frame<20)input->button=PAD_BUTTON_X;
    /* Release X before a second normal jump edge if still short of landing. */
    if(jump_frame && frame-jump_frame>=30 && frame-jump_frame<34 && fp->ground_or_air!=GA_Ground && fp->cur_pos.x<192)input->button=PAD_BUTTON_X;
    if(jump_frame && frame-jump_frame>=30 && fp->cur_pos.x>=190 && fp->cur_pos.x<=225 && fp->cur_pos.y>-12 && fp->cur_pos.y<12 && fp->ground_or_air==GA_Ground){
     gap_landed_frame=frame;memset(input,0,sizeof(*input));OSReport("NATIVE_MAZE_GAP_LANDED frame=%u x=%f y=%f motion=%d floor=%d stocks=%d damage=%d\n",frame,fp->cur_pos.x,fp->cur_pos.y,fp->motion_id,fp->coll_data.floor.index,Player_GetStocks(0),Player_GetDamage(0));
    }
    }else{
     /* Let the original landing animation finish before a fresh jump edge.
      * The next one-way platform is x260..280, y43.56 in the authored map. */
     if(frame-gap_landed_frame>=35 && !raised_jump_frame)input->stickX=80;
     if(!raised_jump_frame && frame-gap_landed_frame>=35 && fp->cur_pos.x>=209 &&
        fp->ground_or_air==GA_Ground){
      raised_jump_frame=frame;OSReport("NATIVE_MAZE_RAISED_JUMP frame=%u x=%f y=%f floor=%d\n",frame,fp->cur_pos.x,fp->cur_pos.y,fp->coll_data.floor.index);
     }
     if(raised_jump_frame){
      unsigned elapsed=frame-raised_jump_frame;
      if(fp->cur_pos.x<268)input->stickX=80;
      /* The hanging block spans x233.58..250.58 above y85.46.
       * Stay below it on the first arc, then jump again after clearing its
       * right wall with body margin. No collision or velocity writes. */
      if(!raised_second_jump && elapsed>=24 && fp->cur_pos.x>=257 && fp->ground_or_air!=GA_Ground){
       raised_second_jump=frame;OSReport("NATIVE_MAZE_RAISED_SECOND_JUMP frame=%u x=%f y=%f\n",frame,fp->cur_pos.x,fp->cur_pos.y);
      }
      if(elapsed<20 || (raised_second_jump && frame-raised_second_jump<4))input->button=PAD_BUTTON_X;
      if(elapsed>12 && fp->ground_or_air==GA_Ground && fp->coll_data.floor.index!=24 && fp->coll_data.floor.index!=27){
       route_aborted=1;memset(input,0,sizeof(*input));OSReport("NATIVE_MAZE_ROUTE_ABORT unexpected_floor=%d frame=%u x=%f y=%f\n",fp->coll_data.floor.index,frame,fp->cur_pos.x,fp->cur_pos.y);
      }
      if(elapsed>=30 && fp->cur_pos.x>=255 && fp->cur_pos.x<=285 &&
         fp->cur_pos.y>40 && fp->cur_pos.y<47 && fp->ground_or_air==GA_Ground){
       route_done=1;memset(input,0,sizeof(*input));OSReport("NATIVE_MAZE_RAISED_LANDED frame=%u x=%f y=%f motion=%d floor=%d stocks=%d damage=%d\n",frame,fp->cur_pos.x,fp->cur_pos.y,fp->motion_id,fp->coll_data.floor.index,Player_GetStocks(0),Player_GetDamage(0));
      }
     }
    }
   }
  }
  if((frame%5==0 || (frame>=440 && frame<=480)) && frame>=175 && frame<=1800)OSReport("NATIVE_MAZE_ROUTE frame=%u x=%f y=%f motion=%d air=%d floor=%d stocks=%d damage=%d requested_x=%d button=%u done=%d aborted=%d\n",frame,fp->cur_pos.x,fp->cur_pos.y,fp->motion_id,fp->ground_or_air,fp->coll_data.floor.index,Player_GetStocks(0),Player_GetDamage(0),input->stickX,input->button,route_done,route_aborted);
  return;
 }
 if(getenv("MELEE_DIAGNOSTIC_ADVENTURE_MAZE_INPUT") &&
    Stage_8022519C(Stage_80225194())==Gr_Kind_ShrineRoute){
  Fighter* fp=self->user_data;
  static unsigned right_frame,left_frame,jump_frame,attack_frame;
  if(!right_frame && frame>=180 && fp->ground_or_air==GA_Ground && fp->motion_id==14){
   right_frame=frame;OSReport("NATIVE_MAZE_READY action=right frame=%u\n",frame);
  }
  if(right_frame && frame-right_frame<20)input->stickX=45;
  if(right_frame && !left_frame && frame-right_frame>=50 && fp->ground_or_air==GA_Ground && fp->motion_id==14){
   left_frame=frame;OSReport("NATIVE_MAZE_READY action=left frame=%u\n",frame);
  }
  if(left_frame && frame-left_frame<20)input->stickX=-45;
  if(left_frame && !jump_frame && frame-left_frame>=50 && fp->ground_or_air==GA_Ground && fp->motion_id==14 && !(fp->input.held_buttons[0]&PAD_BUTTON_X)){
   jump_frame=frame;OSReport("NATIVE_MAZE_READY action=jump frame=%u\n",frame);
  }
  if(jump_frame && frame-jump_frame<4)input->button=PAD_BUTTON_X;
  if(jump_frame && !attack_frame && frame-jump_frame>=90 && fp->ground_or_air==GA_Ground && fp->motion_id==14 && !(fp->input.held_buttons[0]&PAD_BUTTON_A)){
   attack_frame=frame;OSReport("NATIVE_MAZE_READY action=attack frame=%u\n",frame);
  }
  if(attack_frame && frame-attack_frame<4)input->button=PAD_BUTTON_A;
  if(frame%5==0 && frame>=175 && frame<=1200)OSReport("NATIVE_MAZE_INPUT frame=%u motion=%d air=%d stocks=%d damage=%d x=%f y=%f requested_x=%d button=%u\n",frame,fp->motion_id,fp->ground_or_air,Player_GetStocks(0),Player_GetDamage(0),fp->cur_pos.x,fp->cur_pos.y,input->stickX,input->button);
  return;
 }
 if((getenv("MELEE_DIAGNOSTIC_ADVENTURE_KONGO_COMBAT") &&
     Stage_8022519C(Stage_80225194())==Gr_Kind_Kongo) ||
    (getenv("MELEE_DIAGNOSTIC_ADVENTURE_GIANT_DONKEY_COMBAT") &&
     Stage_8022519C(Stage_80225194())==Gr_Kind_Garden)){
  Fighter* fp=self->user_data;Fighter* foe=NULL;int slot=-1;float best=1.0e30f;
  for(int i=1;i<6;i++){
   HSD_GObj* other=Player_GetEntity(i);if(!other || Player_GetStocks(i)<=0)continue;
   Fighter* candidate=other->user_data;if(!candidate || candidate->kind!=Ft_Kind_Donkey)continue;
   float distance=MeleePc_KoopaAbs(candidate->cur_pos.x-fp->cur_pos.x)+MeleePc_KoopaAbs(candidate->cur_pos.y-fp->cur_pos.y);
   if(distance<best){best=distance;foe=candidate;slot=i;}
  }
  if(foe){
   float dx=foe->cur_pos.x-fp->cur_pos.x,dy=foe->cur_pos.y-fp->cur_pos.y;
   int direction=dx>=0?1:-1;unsigned phase=frame%60;
   if(MeleePc_KoopaAbs(dx)>18)input->stickX=(s8)(direction*55);
   if(phase<4 && fp->ground_or_air==GA_Ground && dy>15)input->button=PAD_BUTTON_X;
   else if(phase>=30 && phase<34 && fp->ground_or_air!=GA_Ground && dy>25)input->button=PAD_BUTTON_X;
   if(MeleePc_KoopaAbs(dx)<=35 && dy>-28 && dy<28){
    if((phase>=6 && phase<12)||(phase>=36 && phase<42))input->stickX=0;
    if((phase>=12 && phase<15)||(phase>=42 && phase<45)){input->stickX=(s8)(direction*80);input->button=PAD_BUTTON_A;}
   }
  }
  if(frame%30==0)OSReport("NATIVE_ADVENTURE_KONGO_COMBAT stage=%d frame=%u slot=%d self_stocks=%d self_damage=%d target_stocks=%d target_damage=%d x=%f y=%f motion=%d air=%d target_x=%f target_y=%f requested_x=%d button=%u\n",Stage_8022519C(Stage_80225194()),frame,slot,Player_GetStocks(0),Player_GetDamage(0),slot>=0?Player_GetStocks(slot):-1,slot>=0?Player_GetDamage(slot):-1,fp->cur_pos.x,fp->cur_pos.y,fp->motion_id,fp->ground_or_air,foe?foe->cur_pos.x:0,foe?foe->cur_pos.y:0,input->stickX,input->button);
  return;
 }
 if(getenv("MELEE_DIAGNOSTIC_ADVENTURE_KONGO_INPUT") &&
    Stage_8022519C(Stage_80225194())==Gr_Kind_Kongo){
  Fighter* fp=self->user_data;
  if(frame>=180 && frame<210)input->stickX=45;
  static unsigned kongo_left_frame,kongo_jump_frame,kongo_attack_frame;
  if(!kongo_left_frame && frame>=240 && frame<1800 && fp->ground_or_air==GA_Ground && fp->motion_id==14){
   kongo_left_frame=frame;OSReport("NATIVE_KONGO_LEFT_READY frame=%u motion=%d air=%d x=%f\n",frame,fp->motion_id,fp->ground_or_air,fp->cur_pos.x);
  }
  if(kongo_left_frame && frame-kongo_left_frame<30)input->stickX=-45;
  if(kongo_left_frame && !kongo_jump_frame && frame>=360 && frame-kongo_left_frame>=60 && frame<1800 && fp->ground_or_air==GA_Ground && fp->motion_id==14 && !(fp->input.held_buttons[0]&PAD_BUTTON_X)){
   kongo_jump_frame=frame;OSReport("NATIVE_KONGO_JUMP_READY frame=%u motion=%d air=%d\n",frame,fp->motion_id,fp->ground_or_air);
  }
  if(kongo_jump_frame && frame-kongo_jump_frame<4)input->button=PAD_BUTTON_X;
  if(kongo_jump_frame && !kongo_attack_frame && frame>=540 && frame-kongo_jump_frame>=90 && frame<2400 && fp->ground_or_air==GA_Ground && fp->motion_id==14 && !(fp->input.held_buttons[0]&PAD_BUTTON_A)){
   kongo_attack_frame=frame;OSReport("NATIVE_KONGO_ATTACK_READY frame=%u motion=%d air=%d\n",frame,fp->motion_id,fp->ground_or_air);
  }
  if(kongo_attack_frame && frame-kongo_attack_frame<4)input->button=PAD_BUTTON_A;
  if(frame%30==0 || (frame%5==0 && ((kongo_left_frame && frame-kongo_left_frame<45) || (kongo_jump_frame && frame-kongo_jump_frame<60) || (kongo_attack_frame && frame-kongo_attack_frame<45))))OSReport("NATIVE_ADVENTURE_KONGO_INPUT frame=%u motion=%d air=%d stocks=%d damage=%d x=%f y=%f requested_x=%d button=%u\n",frame,fp->motion_id,fp->ground_or_air,Player_GetStocks(0),Player_GetDamage(0),fp->cur_pos.x,fp->cur_pos.y,input->stickX,input->button);
  return;
 }

 if(getenv("MELEE_DIAGNOSTIC_ADVENTURE_CASTLE_COMBAT") &&
    Stage_8022519C(Stage_80225194())==Gr_Kind_Castle){
  Fighter* fp=self->user_data;Fighter* foe=NULL;int slot=-1;float best=1.0e30f;
  for(int i=1;i<6;i++){
   HSD_GObj* other=Player_GetEntity(i);if(!other || Player_GetStocks(i)<=0)continue;
   Fighter* candidate=other->user_data;
   if(!candidate || (candidate->kind!=Ft_Kind_Mario && candidate->kind!=Ft_Kind_Luigi && candidate->kind!=Ft_Kind_Peach))continue;
   float distance=MeleePc_KoopaAbs(candidate->cur_pos.x-fp->cur_pos.x)+MeleePc_KoopaAbs(candidate->cur_pos.y-fp->cur_pos.y);
   if(distance<best){best=distance;foe=candidate;slot=i;}
  }
  if(foe){
   float dx=foe->cur_pos.x-fp->cur_pos.x,dy=foe->cur_pos.y-fp->cur_pos.y;
   int direction=dx>=0?1:-1;unsigned phase=frame%60;
   if(MeleePc_KoopaAbs(dx)>18)input->stickX=(s8)(direction*55);
   if(phase<4 && fp->ground_or_air==GA_Ground && dy>15)input->button=PAD_BUTTON_X;
   else if(phase>=30 && phase<34 && fp->ground_or_air!=GA_Ground && dy>25)input->button=PAD_BUTTON_X;
   /* Retail OttottoWait requires stronger walking or a fresh jump edge.
    * Release then jump toward a lower opponent; no collision/state edits. */
   if(dy<-28 && fp->ground_or_air==GA_Ground && (fp->motion_id==245 || fp->motion_id==246)){
    input->stickX=phase<6?0:(s8)(direction*80);
    if(phase>=6 && phase<10)input->button=PAD_BUTTON_X;
    if(phase==6)OSReport("NATIVE_CASTLE_TEETER_EXIT frame=%u motion=%d x=%f y=%f target_x=%f target_y=%f direction=%d\n",frame,fp->motion_id,fp->cur_pos.x,fp->cur_pos.y,foe->cur_pos.x,foe->cur_pos.y,direction);
   }
   if(MeleePc_KoopaAbs(dx)<=35 && dy>-28 && dy<28){
    if((phase>=6 && phase<12)||(phase>=36 && phase<42))input->stickX=0;
    if((phase>=12 && phase<15)||(phase>=42 && phase<45)){input->stickX=(s8)(direction*80);input->button=PAD_BUTTON_A;}
   }
  }
  if(frame%30==0)OSReport("NATIVE_ADVENTURE_CASTLE_COMBAT frame=%u slot=%d self_stocks=%d self_damage=%d target_stocks=%d target_damage=%d x=%f y=%f motion=%d requested_x=%d button=%u\n",frame,slot,Player_GetStocks(0),Player_GetDamage(0),slot>=0?Player_GetStocks(slot):-1,slot>=0?Player_GetDamage(slot):-1,fp->cur_pos.x,fp->cur_pos.y,fp->motion_id,input->stickX,input->button);
  return;
 }
 if(Stage_8022519C(Stage_80225194())!=Gr_Kind_KinokoRoute)return;
 Fighter* fp=self->user_data;Item* nearest=NULL;HSD_GObj* target=NULL;
 /* Once original fighters spawn, traversal must never steer through the
  * arena. Use ordinary main-stick/A combat, as in the Classic probe. */
 if(getenv("MELEE_DIAGNOSTIC_ADVENTURE_YOSHI_COMBAT")){
  static int fight_started;HSD_GObj* opponent=NULL;float best=1.0e30f;int slot=-1;
  for(int i=1;i<6;i++){
   HSD_GObj* other=Player_GetEntity(i);if(!other || Player_GetStocks(i)<=0)continue;
   Fighter* foe=other->user_data;if(!foe || foe->kind!=14)continue;
   float score=MeleePc_KoopaAbs(foe->cur_pos.x-fp->cur_pos.x)+MeleePc_KoopaAbs(foe->cur_pos.y-fp->cur_pos.y);
   if(score<best){best=score;opponent=other;slot=i;}
  }
  if(opponent && !fight_started){fight_started=1;OSReport("NATIVE_ADVENTURE_YOSHI_HANDOFF frame=%u x=%f y=%f\n",frame,fp->cur_pos.x,fp->cur_pos.y);}
  if(fight_started){
   /* The original horde callback sets this only after its victory. Missing
    * opponents between waves alone must never resume traversal. */
   if(!opponent && Ground_801C5764()==1){
    static int completed;static unsigned stuck_frame,jump_until;static float last_x;
    if(!completed){completed=1;stuck_frame=frame;last_x=fp->cur_pos.x;OSReport("NATIVE_ADVENTURE_YOSHI_COMPLETE frame=%u original_result=1 stocks=%d x=%f y=%f\n",frame,Player_GetStocks(0),fp->cur_pos.x,fp->cur_pos.y);}
    if(getenv("MELEE_DIAGNOSTIC_LUIGI_FLAG_TIMING") && fp->cur_pos.x>=1000){
     static int waiting, released, prior_seconds;int seconds=0;
     if(!released && GetMatchTimer(&seconds)){
      if(!waiting){waiting=1;prior_seconds=seconds;OSReport("NATIVE_LUIGI_FLAG_WAIT frame=%u seconds=%d x=%f\n",frame,seconds,fp->cur_pos.x);}
      if(seconds!=prior_seconds && seconds%10==2){released=1;OSReport("NATIVE_LUIGI_FLAG_RELEASE frame=%u seconds=%d x=%f\n",frame,seconds,fp->cur_pos.x);}
      prior_seconds=seconds;
      if(!released)return;
     }
    }
    input->stickX=80;
    if(fp->ground_or_air==GA_Ground && frame-stuck_frame>=45){
     if(MeleePc_KoopaAbs(fp->cur_pos.x-last_x)<6)jump_until=frame+20;
     last_x=fp->cur_pos.x;stuck_frame=frame;
    }
    if(frame<jump_until)input->button=PAD_BUTTON_X;
    if(frame%60==0)OSReport("NATIVE_ADVENTURE_POST_YOSHI frame=%u stocks=%d x=%f y=%f motion=%d\n",frame,Player_GetStocks(0),fp->cur_pos.x,fp->cur_pos.y,fp->motion_id);
    return;
   }
   if(opponent){
    Fighter* foe=opponent->user_data;float dx=foe->cur_pos.x-fp->cur_pos.x,dy=foe->cur_pos.y-fp->cur_pos.y;
    int direction=dx>=0?1:-1;unsigned phase=frame%60;
    if(MeleePc_KoopaAbs(dx)>18)input->stickX=(s8)(direction*55);
    if(phase<4 && fp->ground_or_air==GA_Ground && dy>15)input->button=PAD_BUTTON_X;
    else if(phase>=30 && phase<34 && fp->ground_or_air!=GA_Ground && dy>25)input->button=PAD_BUTTON_X;
    if(MeleePc_KoopaAbs(dx)<=35 && dy>-28 && dy<28){
     if((phase>=6 && phase<12)||(phase>=36 && phase<42))input->stickX=0;
     if((phase>=12 && phase<15)||(phase>=42 && phase<45)){input->stickX=(s8)(direction*80);input->button=PAD_BUTTON_A;}
    }
    if(frame%30==0)OSReport("NATIVE_ADVENTURE_YOSHI_COMBAT frame=%u slot=%d self_stocks=%d target_stocks=%d target_damage=%d x=%f y=%f target_x=%f target_y=%f\n",frame,slot,Player_GetStocks(0),Player_GetStocks(slot),Player_GetDamage(slot),fp->cur_pos.x,fp->cur_pos.y,foe->cur_pos.x,foe->cur_pos.y);
   }
   return;
  }
 }
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
  if(getenv("MELEE_DIAGNOSTIC_EXIT_MARKERS")){
   for(int marker=0xBD;marker<0xC7;marker++){
    Vec3 exit_pos;
    if(Ground_801C2D24(marker,&exit_pos))
     OSReport("NATIVE_ADVENTURE_EXIT_MARKER id=%d x=%f y=%f z=%f\n",marker,exit_pos.x,exit_pos.y,exit_pos.z);
   }
  }

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
 if(getenv("MELEE_DIAGNOSTIC_SHELL_INTERACTION") && collapse_seen && frame<6600 && Player_GetStocks(0)>0){
  static unsigned phase,pickup_start,held_frame,release_frame;static HSD_GObj* held_shell;
  Item* held=fp->item_gobj?fp->item_gobj->user_data:NULL;
  int held_is_shell=held && (held->kind==It_Kind_ZGShell || held->kind==It_Kind_ZRShell);
  memset(input,0,sizeof(*input));
  /* A pickup can select a nearby ordinary item before the shell. The
   * original throw check requires A while L/R is held for usable items. */
  if(held && !held_is_shell){
   pickup_start=0;
   if(frame%30<4){input->button=PAD_TRIGGER_L|PAD_BUTTON_A;input->stickX=-80;}
   if(frame%30==0)OSReport("NATIVE_SHELL_UNEXPECTED_ITEM frame=%u kind=%d motion=%d discard=%u\n",frame,held->kind,fp->motion_id,input->button);
   return;
  }
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
  if(phase==4 && getenv("MELEE_DIAGNOSTIC_POST_SHELL_TRAVERSE") && frame>=release_frame+180 && frame<6600){
   static float route_x;static unsigned route_frame,route_jump_until;
   static int gap_jump_done, step_jump_done, exit_jump_done, exit_double_done;
   static unsigned exit_jump_frame;static int route_stocks=-1;
   if(route_stocks!=Player_GetStocks(0)){
    route_stocks=Player_GetStocks(0);gap_jump_done=step_jump_done=exit_jump_done=exit_double_done=0;
    route_frame=frame;route_x=fp->cur_pos.x;route_jump_until=0;
   }
   input->stickX=80;
   if(!gap_jump_done && fp->ground_or_air==GA_Ground && fp->cur_pos.x>=260.0f){gap_jump_done=1;route_jump_until=frame+20;OSReport("NATIVE_POST_SHELL_GAP_JUMP frame=%u x=%f y=%f\n",frame,fp->cur_pos.x,fp->cur_pos.y);}
   if(!step_jump_done && fp->ground_or_air==GA_Ground && (fp->motion_id==16 || fp->motion_id==17) && fp->cur_pos.x>=320.0f && fp->cur_pos.x<380.0f && fp->cur_pos.y>60.0f){step_jump_done=1;route_jump_until=frame+20;OSReport("NATIVE_POST_SHELL_STEP_JUMP frame=%u x=%f y=%f\n",frame,fp->cur_pos.x,fp->cur_pos.y);}
   /* GrNKr collision is scaled to world coordinates. The second ground
    * island ends at x458; jump while still standing on its right side. */
   if(!exit_jump_done && fp->ground_or_air==GA_Ground && (fp->motion_id==16 || fp->motion_id==17) && fp->cur_pos.x>=430.0f && fp->cur_pos.x<450.0f && fp->cur_pos.y>5.0f && fp->cur_pos.y<30.0f){exit_jump_done=1;exit_jump_frame=frame;route_jump_until=frame+20;OSReport("NATIVE_POST_SHELL_EXIT_JUMP frame=%u x=%f y=%f\n",frame,fp->cur_pos.x,fp->cur_pos.y);}
   if(exit_jump_done && !exit_double_done && frame>=exit_jump_frame+27 && fp->ground_or_air!=GA_Ground){exit_double_done=1;route_jump_until=frame+4;OSReport("NATIVE_POST_SHELL_EXIT_DOUBLE frame=%u x=%f y=%f\n",frame,fp->cur_pos.x,fp->cur_pos.y);}
   if(!route_frame){route_frame=frame;route_x=fp->cur_pos.x;}
   if(fp->ground_or_air==GA_Ground && frame-route_frame>=45){
    if(fp->cur_pos.x-route_x<6)route_jump_until=frame+20;
    route_x=fp->cur_pos.x;route_frame=frame;
   }
   if(frame<route_jump_until)input->button=PAD_BUTTON_X;
   if(frame>=1925 && frame<=1985 && frame%5==0){
    CollJoint* joint=&mpGetGroundCollJoint()[57];
    CollLine* line=&mpGetGroundCollLine()[107];
    CollVtx* vertices=mpGetGroundCollVtx();
    MapLine* map_line=line->x0;
    OSReport("NATIVE_EXIT_FLOOR_TRACE frame=%u x=%f y=%f motion=%d air=%d joint_flags=%08x line_flags=%08x lo=%04x v0=(%f,%f) v1=(%f,%f)\n",frame,fp->cur_pos.x,fp->cur_pos.y,fp->motion_id,fp->ground_or_air,joint->flags,line->flags,map_line->lo_flags,vertices[map_line->v0_idx].pos.x,vertices[map_line->v0_idx].pos.y,vertices[map_line->v1_idx].pos.x,vertices[map_line->v1_idx].pos.y);
   }
   if(frame%30==0)OSReport("NATIVE_POST_SHELL_TRAVERSE frame=%u x=%f y=%f motion=%d stocks=%d stick=%d jump=%u\n",frame,fp->cur_pos.x,fp->cur_pos.y,fp->motion_id,Player_GetStocks(0),input->stickX,frame<route_jump_until);
  }

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

