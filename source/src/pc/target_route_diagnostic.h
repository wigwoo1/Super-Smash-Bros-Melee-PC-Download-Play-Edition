#ifndef MELEE_PC_TARGET_ROUTE_DIAGNOSTIC_H
#define MELEE_PC_TARGET_ROUTE_DIAGNOSTIC_H
#include <melee/gr/ground.h>
#include <melee/gr/types.h>
#include <melee/it/itCommonItems.h>
#include <melee/it/types.h>
#include <melee/mp/mplib.h>
#include <sysdolphin/baselib/gobj.h>
#include <stdlib.h>

/* Read-only identity: original Mato spawn stores the original stage joint. */
static HSD_GObj* MeleePc_TargetRouteFind(int point, Vec3* position)
{
    if (point<199 || point>=220 || !stage_info.x280[point]) return NULL;
    for (HSD_GObj* g=HSD_GObjPLinkHead[HSD_GOBJ_PLINK_ITEM];g;g=g->next) {
        Item* item=g->user_data;
        if (item && item->kind==It_Kind_Mato &&
            item->xDD4_itemVar.mato.x0==stage_info.x280[point]) {
            if (position) *position=item->pos;
            return g;
        }
    }
    return NULL;
}

static void MeleePc_TargetRouteSurvey(unsigned frame, PADStatus* input, int ready)
{
    static unsigned begin;
    if (!begin && ready) {
        begin=frame;
        OSReport("NATIVE_TARGET_SURVEY_BEGIN frame=%u remaining=%d\n",frame,stage_info.x6D4);
    }
    if (!begin) return;
    memset(input,0,sizeof(*input));
    if ((frame-begin)%30==0) {
        for (int i=0;i<11;++i) {
            Vec3 left,right;mpFloorGetLeft(i,&left);mpFloorGetRight(i,&right);
            OSReport("NATIVE_TARGET_SURVEY_FLOOR frame=%u line=%d left=%f right=%f y=%f\n",
                     frame,i,left.x,right.x,(left.y+right.y)*0.5f);
        }
        for (int i=199;i<220;++i) {
            Vec3 pos;HSD_GObj* item=MeleePc_TargetRouteFind(i,&pos);
            if (item) OSReport("NATIVE_TARGET_SURVEY_ITEM frame=%u point=%d item=%p x=%f y=%f\n",
                               frame,i,item,pos.x,pos.y);
        }
    }
    if (frame-begin==1800) {
        int queued=aurora_diagnostic_queue_quit();
        OSReport("NATIVE_TARGET_SURVEY_CLOSE frame=%u remaining=%d queued=%d\n",frame,stage_info.x6D4,queued);
    }
}
/* Opt-in controller route only. All coordinates below are controller goals,
 * never writes to fighter, collision, item, stage or scene state. */
static f32 MeleePc_TargetRouteAbs(f32 x) { return x<0?-x:x; }
static int MeleePc_TargetRouteSteer(Fighter* fp, f32 x, int slow)
{
    f32 dx=x-fp->cur_pos.x;
    f32 vx=fp->self_vel.x;
    if (fp->ground_or_air==GA_Air && dx*vx>0 &&
        MeleePc_TargetRouteAbs(dx)<vx*vx/0.075f+2)
        return vx>0?-80:80;
    if (dx>2) return slow?55:80;
    if (dx<-2) return slow?-55:-80;
    return fp->self_vel.x>0.08f?-35:fp->self_vel.x<-0.08f?35:0;
}

static void MeleePc_TargetRouteAll(unsigned frame, HSD_GObj* self,
                                  PADStatus* input, int ready)
{
    enum { RouteNav, RouteAttack, RouteDrop, RoutePill, RouteSlot, RouteLast, RouteLeftShelf };
    static const struct { int kind, floor, point; } route[]={
        {RouteNav,4,0},{RouteAttack,4,203},
        {RouteNav,5,0},{RouteLeftShelf,1,0},{RouteAttack,1,201},
        {RouteNav,7,0},{RouteNav,4,0},{RouteNav,5,0},{RouteLeftShelf,1,0},
        {RouteDrop,2,0},{RoutePill,2,202},{RouteAttack,2,200},
        {RouteNav,7,0},{RouteNav,4,0},{RouteNav,8,0},{RouteNav,0,0},
        {RouteAttack,0,199},{RouteNav,9,0},{RouteAttack,9,206},
        {RouteSlot,10,0},{RouteAttack,10,205},
        {RouteNav,9,0},{RouteLast,9,207}
    };
    static unsigned begin, step, step_begin, stable, jump_begin, action_begin;
    static int airborne, attacked, hit, closing, jump_accepted;
    static int previous_floor=-1, special_direction;
    static f32 previous_center;
    Fighter* fp=self->user_data;
    Vec3 pos=fp->cur_pos,left,right,goal={0};
    int ground=ftLib_GetGroundAir(self)==GA_Ground;
    int motion=ftLib_GetMotionId(self);
    if (!begin && ready) {
        begin=step_begin=frame;
        OSReport("NATIVE_TARGET_ALL_BEGIN frame=%u remaining=%d stocks=%d\n",frame,stage_info.x6D4,Player_GetStocks(0));
    }
    if (!begin || closing) return;
    memset(input,0,sizeof(*input));
    if (!stage_info.x6D4) return; /* Original game owns completion. */
    if (step>=sizeof(route)/sizeof(route[0]) || frame-step_begin>900 ||
        Player_GetStocks(0)!=1) {
        closing=1;
        int queued=aurora_diagnostic_queue_quit();
        OSReport("NATIVE_TARGET_ALL_ABORT frame=%u step=%u remaining=%d stocks=%d reason=route_bound queued=%d\n",
                 frame,step,stage_info.x6D4,Player_GetStocks(0),queued);
        return;
    }
    int kind=route[step].kind, floor=route[step].floor, point=route[step].point;
    mpFloorGetLeft(floor,&left);mpFloorGetRight(floor,&right);
    f32 center=(left.x+right.x)*0.5f, height=(left.y+right.y)*0.5f;
    f32 velocity=previous_floor==floor?center-previous_center:0;
    previous_floor=floor;previous_center=center;
    f32 landing=center+velocity*45;
    if (floor==1 || floor==2) {
        if (landing<-105) landing=-105;
        if (landing>-60) landing=-60;
    } else if (floor==4) {
        if (landing<-65) landing=-65;
        if (landing>-20) landing=-20;
    }
    /* The lower left shelf slides through the fixed column. Its geometric
     * center is not a walkable goal while approaching from the west. */
    if (floor==2) {
        if (center>-103) center=-103;
        if (landing>-103) landing=-103;
    }
    int on_floor=ground && fp->coll_data.floor.index==floor;
    int alive=point?MeleePc_TargetRouteFind(point,&goal)!=NULL:0;
    int finished=0;
    unsigned elapsed=frame-step_begin;
    /* Preserve the verified first-four-target prefix. Later waypoints must
     * finish original landing recovery before counting stable/launch ticks. */
    if (step>=5 && ground && !jump_begin &&
        (motion==ftCo_MS_Landing || motion==ftCo_MS_LandingFallSpecial)) {
        if (frame%5==0)
            OSReport("NATIVE_TARGET_ALL_LANDING_WAIT frame=%u step=%u motion=%d floor=%d\n",
                     frame,step,motion,fp->coll_data.floor.index);
        return;
    }
    if (step==8 && ground && fp->coll_data.floor.index==2) {
        /* A missed upper shelf can still produce a valid lower-shelf landing.
         * Skip only diagnostic waypoints, never original targets or state. */
        OSReport("NATIVE_TARGET_ALL_ALTERNATE frame=%u from_step=%u to_step=10 floor=2 remaining=%d stocks=%d\n",
                 frame,step,stage_info.x6D4,Player_GetStocks(0));
        step=10;step_begin=frame;stable=jump_begin=action_begin=0;
        airborne=attacked=hit=special_direction=jump_accepted=0;
        return;
    }
    if (kind==RouteAttack && alive && on_floor && point==201 && !jump_begin && left.x>-115) {
        input->stickX=MeleePc_TargetRouteSteer(fp,center,1);
    } else if (kind==RouteAttack && alive && on_floor && point==199 && !jump_begin && height<38) {
        /* Be at the launch edge before the short high-elevator window. */
        input->stickX=MeleePc_TargetRouteSteer(fp,right.x-4,1);
    } else if (kind==RouteAttack && alive && on_floor && point==203 && !jump_begin && right.x<-14) {
        input->stickX=MeleePc_TargetRouteSteer(fp,center,1);
    } else if (kind==RouteLeftShelf) {
        /* Floor5 is the fixed column that blocks walking along floor1.
         * Step off its left side only while the moving shelf is underneath. */
        if (on_floor && pos.x<-98) {
            if (++stable>=5) finished=1;
        } else if (ground) {
            if (step>=8) {
                /* Begin the descent while shelves move west, leaving enough
                 * support time for the 90-unit drop and subsequent attack. */
                if (!action_begin && left.x<-90 && velocity<-0.1f) action_begin=frame;
                if (action_begin) input->stickX=-62;
            } else if (left.x<-110) input->stickX=-62;
        } else {
            f32 shelf_aim=left.x+10;
            if (step>=8 && shelf_aim>-101) shelf_aim=-101;
            input->stickX=MeleePc_TargetRouteSteer(fp,shelf_aim,0);
            if (pos.y<height-10 && elapsed%2==0) {
                if (fp->x1968_jumpsUsed<fp->co_attrs.max_jumps) input->button=PAD_BUTTON_X;
                else { input->stickY=80;input->button=PAD_BUTTON_B; }
            }
        }
    } else if (kind==RoutePill) {
        /* Original downward pill flight; wait for a support window and face
         * left before neutral B. Projectile callbacks provide hit identity. */
        /* Projectile travel can outlast the moving support. The next phase
         * must receive control in air as soon as the original hit confirms. */
        if (!alive) finished=1;
        else if (on_floor) {
            if (!action_begin) {
                if (left.x<-106 && right.x>-97) {
                    if (MeleePc_TargetRouteAbs(pos.x+101)>2)
                        input->stickX=MeleePc_TargetRouteSteer(fp,-101,1);
                    else if (fp->facing_dir>0) input->stickX=-30;
                    else action_begin=frame;
                } else input->stickX=MeleePc_TargetRouteSteer(fp,center,1);
            } else {
                unsigned shot=(frame-action_begin)%90;
                if (shot>=6 && shot<9) input->button=PAD_BUTTON_B;
                if (shot==89) action_begin=0;
            }
        }
    } else if (kind==RouteDrop) {
        if (on_floor) { if (++stable>=20) finished=1; }
        else {
            /* Stay west of the solid column while dropping between the
             * two left moving shelves; their centers can pass into it. */
            input->stickX=MeleePc_TargetRouteSteer(fp,floor==2?-105:center,0);
            if (ground && elapsed%12<3) input->stickY=-80;
        }
    } else if (kind==RouteSlot) {
        if (on_floor) { if (++stable>=20) finished=1; }
        else if (ground) input->stickX=-62;
        else {
            /* Once below the lip, keep returning into the slot even when
             * the aerial jump rises above the approach-height threshold. */
            if (!action_begin && pos.y<=-99) action_begin=frame;
            f32 x=action_begin?center:20;
            input->stickX=MeleePc_TargetRouteSteer(fp,x,0);
            if (action_begin && fp->x1968_jumpsUsed<fp->co_attrs.max_jumps && elapsed%2==0) {
                input->button=PAD_BUTTON_X;input->stickX=80;
            }
        }
    } else if (kind==RouteLast) {
        /* Leave the lower-right target until last: its original accepted
         * hit must complete the stage before any recovery is needed. */
        /* Exit the inner floor to the right beneath the outer lip at x80,
         * y-50. Preserve the air jump until its ECB has cleared that wall. */
        input->stickX=MeleePc_TargetRouteSteer(fp,goal.x-7,0);
        if (ground) input->stickX=62;
        else if (!action_begin && pos.x>84 && pos.y<-65 &&
                 fp->x1968_jumpsUsed<fp->co_attrs.max_jumps) {
            if (elapsed%2==0) {
                input->stickX=80;input->button=PAD_BUTTON_X;
            }
        } else if (!action_begin && pos.x>100 && pos.y<goal.y-12 &&
                   fp->self_vel.y<=0 &&
                   fp->x1968_jumpsUsed>=fp->co_attrs.max_jumps)
            action_begin=frame;
        if (action_begin) {
            input->stickX=35;
            if ((frame-action_begin)%2==0) {
                input->stickY=80;input->button=PAD_BUTTON_B;
            }
        }
    } else if (step==21 && kind==RouteNav && !on_floor) {
        /* Floor9 has a solid underside above the narrow floor10 slot.
         * Exit left, use the air jump outside x25, then recover inward. */
        if (ground) input->stickX=-62;
        else {
            int special=motion==ftMr_MS_SpecialAirHi ||
                motion==ftCo_MS_FallSpecial || motion==ftCo_MS_FallSpecialF ||
                motion==ftCo_MS_FallSpecialB;
            input->stickX=MeleePc_TargetRouteSteer(fp,special?37.5f:18,0);
            if (motion==ftMr_MS_SpecialAirHi) input->stickX=35;
            else if (!special && pos.x<21) {
                if (fp->x1968_jumpsUsed<fp->co_attrs.max_jumps) {
                    if (elapsed%2==0) {
                        input->button=PAD_BUTTON_X;input->stickX=0;
                    }
                } else if (fp->self_vel.y<=0 && elapsed%2==0) {
                    input->stickX=35;input->stickY=80;input->button=PAD_BUTTON_B;
                }
            }
        }
    } else if (step==17 && kind==RouteNav && !on_floor) {
        /* Floor9 is inside the hollow central structure. Its outer wall ends
         * at (80,-50), with an underside extending to x70. Go beneath that
         * lip before spending the air jump; a direct jump hits the slope. */
        if (ground) {
            input->stickX=-62;
        } else {
            input->stickX=MeleePc_TargetRouteSteer(fp,center,0);
            int special=motion==ftMr_MS_SpecialAirHi ||
                motion==ftCo_MS_FallSpecial || motion==ftCo_MS_FallSpecialF ||
                motion==ftCo_MS_FallSpecialB;
            if (motion==ftMr_MS_SpecialAirHi) input->stickX=-35;
            else if (!special) {
                if (pos.x<69 && pos.y<-65 &&
                    fp->x1968_jumpsUsed<fp->co_attrs.max_jumps) {
                    if (elapsed%2==0) {
                        input->button=PAD_BUTTON_X;input->stickX=-80;
                    }
                } else if (fp->x1968_jumpsUsed>=fp->co_attrs.max_jumps &&
                           fp->self_vel.y<=0 && pos.x<66 && elapsed%2==0) {
                    input->stickX=-35;input->stickY=80;input->button=PAD_BUTTON_B;
                }
            }
        }
    } else {
        if (kind==RouteAttack && !alive) hit=1;
        if (kind==RouteAttack && point==205 && hit && ground &&
            fp->coll_data.floor.index==9) {
            /* An inward recovery may finish on the upper slot platform. */
            input->stickX=MeleePc_TargetRouteSteer(fp,37.5f,1);
            if (MeleePc_TargetRouteAbs(pos.x-37.5f)<5 && ++stable>=20) finished=1;
        } else if (kind==RouteAttack && (point==201 || point==200) && hit && ground &&
            fp->coll_data.floor.index==6) {
            /* Original UpB misses the departing upper shelf but lands on
             * floor6. Accept this real landing; climb via floor7 next. */
            input->stickX=MeleePc_TargetRouteSteer(fp,-65,1);
            if (MeleePc_TargetRouteAbs(pos.x+65)<5 && ++stable>=20) finished=1;
        } else if (on_floor && (kind==RouteNav || hit)) {
            input->stickX=MeleePc_TargetRouteSteer(fp,center,1);
            if (MeleePc_TargetRouteAbs(pos.x-center)<5 && ++stable>=20) finished=1;
        } else {
            stable=0;
            if (!ground) airborne=1;
            if (ground && airborne) { jump_begin=0;airborne=jump_accepted=0; }
            f32 aim=kind==RouteAttack && !hit?goal.x:landing;
            if (point==200 && hit) {
                /* Return immediately toward the fixed lower floor. Original
                 * column collision holds us outside until below its bottom;
                 * braking west first loses the remaining landing window. */
                aim=-65;
            }
            if (kind==RouteAttack && point==203 && !hit) aim=goal.x+12;
            if (kind==RouteAttack && point==201 && !hit) aim=goal.x-7;
            if (kind==RouteAttack && point==199 && !hit) aim=goal.x+7;
            /* Solid high platforms must be approached around their sides.
             * Live floor8 spans[-30,40]; never steer through its underside. */
            if (kind==RouteNav && floor==8 && pos.y<height+3) aim=left.x-7;
            if (kind==RouteNav && floor==5 && pos.y<height+3) aim=right.x+7;
            if (kind==RouteNav && floor==7 && pos.y<height+3) aim=left.x-7;
            if (kind==RouteAttack && ground && !jump_begin) {
                f32 launch=goal.x<center?left.x+4:right.x-4;
                if (point==200 || point==205) launch=goal.x;
                if (MeleePc_TargetRouteAbs(pos.x-launch)>2) {
                    input->stickX=MeleePc_TargetRouteSteer(fp,launch,1);
                } else jump_begin=frame;
            } else if (ground && !jump_begin) jump_begin=frame;
            if (step>=10 && !ground && !jump_begin) {
                jump_begin=frame-8;airborne=1;
                OSReport("NATIVE_TARGET_ALL_AIR_HANDOFF frame=%u step=%u motion=%d jumps=%d\n",
                         frame,step,motion,fp->x1968_jumpsUsed);
            }
            if (jump_begin) {
                if (motion==ftCo_MS_KneeBend && !jump_accepted) {
                    jump_accepted=1;
                    OSReport("NATIVE_TARGET_ALL_JUMP_ACCEPTED frame=%u step=%u request=%u floor=%d\n",
                             frame,step,jump_begin,fp->coll_data.floor.index);
                }
                if (step>=5 && ground && !jump_accepted && frame-jump_begin>=8) {
                    OSReport("NATIVE_TARGET_ALL_JUMP_RETRY frame=%u step=%u motion=%d\n",frame,step,motion);
                    jump_begin=0;
                    memset(input,0,sizeof(*input)); /* release before fresh edge */
                    return;
                }
                input->stickX=MeleePc_TargetRouteSteer(fp,aim,0);
                if (frame-jump_begin<8 || (step>=5 && motion==ftCo_MS_KneeBend)) input->button=PAD_BUTTON_X;
                else if (!ground) {
                    f32 dy=(kind==RouteAttack && !hit?goal.y:height)-pos.y;
                    f32 dx=aim-pos.x;
                    int special=motion==ftMr_MS_SpecialAirHi ||
                        motion==ftCo_MS_FallSpecial || motion==ftCo_MS_FallSpecialF ||
                        motion==ftCo_MS_FallSpecialB;
                    if (motion==ftMr_MS_SpecialAirHi) {
                        input->stickX=special_direction*35;
                        if (point==201 && hit) input->stickX=80;
                    } else if (!special) {
                        int close=MeleePc_TargetRouteAbs(dx)<14;
                        /* Target203 is alongside the first-jump apex. Use
                         * the original back aerial after passing it, keeping
                         * the air jump and UpB available for the return. */
                        if (kind==RouteAttack && point==203 && !hit) {
                            /* Waiting for this aerial is an exclusive phase:
                             * do not spend jump/UpB through a fallback. */
                            if (!action_begin && pos.x>goal.x+2 && dy<8 && dy>-15) {
                                action_begin=frame;
                                OSReport("NATIVE_TARGET_ALL_AERIAL_REQUEST frame=%u point=%d motion=%d x=%f y=%f jumps=%d facing=%f\n",
                                         frame,point,motion,pos.x,pos.y,fp->x1968_jumpsUsed,fp->facing_dir);
                            }
                            if (action_begin) {
                                input->stickX=-80;
                                if (frame-action_begin<3) input->button=PAD_BUTTON_A;
                                attacked=1;
                            }
                        } else if (kind==RouteAttack && point==201 && !hit) {
                            /* Pass to the target's left, then use original
                             * UpB facing back toward the landing platform. */
                            if (pos.x<goal.x-5 && dy>5 && dy<40 && elapsed%2==0) {
                                input->stickX=35;input->stickY=80;input->button=PAD_BUTTON_B;
                                special_direction=1;attacked=1;
                            } else if (fp->self_vel.y<=0 && dy>=40 &&
                                       fp->x1968_jumpsUsed<fp->co_attrs.max_jumps && elapsed%2==0)
                                input->button=PAD_BUTTON_X;
                        } else if (kind==RouteAttack && !hit && close && dy>10 && dy<34) {
                            int attack_direction=(point==200 || point==205)?1:point==199?-1:dx>=0?1:-1;
                            input->stickX=attack_direction*35;
                            if (elapsed%2==0) {
                                input->stickY=80;input->button=PAD_BUTTON_B;special_direction=attack_direction;
                            }
                            attacked=1;
                        } else if (fp->self_vel.y<=0) {
                            if (fp->x1968_jumpsUsed<fp->co_attrs.max_jumps &&
                                (dy>25 || MeleePc_TargetRouteAbs(dx)>20)) {
                                if (elapsed%2==0) {
                                    input->button=PAD_BUTTON_X;
                                    /* The original aerial jump samples stick
                                     * direction for initial horizontal speed.
                                     * Braking here would launch away from199. */
                                    if (kind==RouteAttack && point==199 && !hit)
                                        input->stickX=80;
                                }
                            } else if ((dy>0 || pos.y<height+8) &&
                                       !(kind==RouteNav && floor==0 &&
                                         pos.x>left.x+2 && pos.x<right.x-2 &&
                                         fp->coll_data.cur_pos.y+fp->coll_data.ecb.bottom.y>=height)) {
                                /* Original floor collision uses the world ECB
                                 * bottom; the animation root can be below the
                                 * platform while that contact point is above. */
                                input->stickX=dx>=0?35:-35;
                                if (elapsed%2==0) {
                                    special_direction=kind==RouteNav && floor==5?-1:
                                        kind==RouteNav && (floor==8 || floor==7)?1:
                                        kind==RouteAttack && point==205?1:
                                        kind==RouteAttack && point==199 && !hit?-1:dx>=0?1:-1;
                                    input->stickX=special_direction*35;
                                    input->stickY=80;input->button=PAD_BUTTON_B;
                                }
                                attacked=kind==RouteAttack;
                            }
                        }
                    }
                }
            }
        }
    }
    if (floor==0 && MeleePc_TargetRouteAbs(pos.y-height)<25)
        OSReport("NATIVE_TARGET_ELEVATOR_CONTACT frame=%u step=%u ground=%d motion=%d root_y=%f coll_y=%f bottom_offset=%f bottom_world=%f floor_y=%f floor_index=%d button=%u\n",
                 frame,step,ground,motion,pos.y,fp->coll_data.cur_pos.y,
                 fp->coll_data.ecb.bottom.y,fp->coll_data.cur_pos.y+fp->coll_data.ecb.bottom.y,
                 height,fp->coll_data.floor.index,input->button);
    if (frame%5==0 || input->button || finished)
        OSReport("NATIVE_TARGET_ALL frame=%u step=%u kind=%d floor_goal=%d point=%d alive=%d hit=%d attacked=%d ground=%d floor=%d motion=%d x=%f y=%f vx=%f vy=%f jumps=%d remaining=%d goal_x=%f goal_y=%f floor_left=%f floor_right=%f floor_y=%f button=%u stick_x=%d stick_y=%d\n",
                 frame,step,kind,floor,point,alive,hit,attacked,ground,fp->coll_data.floor.index,motion,
                 pos.x,pos.y,fp->self_vel.x,fp->self_vel.y,fp->x1968_jumpsUsed,stage_info.x6D4,
                 goal.x,goal.y,left.x,right.x,height,input->button,input->stickX,input->stickY);
    if (point==203 && (frame%5==0 || input->button))
        OSReport("NATIVE_TARGET_ALL_ACCEPTANCE frame=%u motion=%d anim=%f interrupt=%d facing=%f held=%u pressed=%u latched_x=%f latched_y=%f jumps=%d\n",
                 frame,motion,fp->cur_anim_frame,fp->allow_interrupt,fp->facing_dir,
                 fp->input.held_buttons[0],fp->input.pressed_buttons,
                 fp->input.lstick[0].x,fp->input.lstick[0].y,fp->x1968_jumpsUsed);
    if (finished) {
        OSReport("NATIVE_TARGET_ALL_STEP frame=%u step=%u floor=%d point=%d remaining=%d stocks=%d\n",
                 frame,step,floor,point,stage_info.x6D4,Player_GetStocks(0));
        ++step;step_begin=frame;stable=jump_begin=action_begin=0;
        airborne=attacked=hit=special_direction=jump_accepted=0;
    }
}
#endif
