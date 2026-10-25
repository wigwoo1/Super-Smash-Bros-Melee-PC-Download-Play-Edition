#ifndef MELEE_PC_FIGURE_CATCH_DIAGNOSTIC_H
#define MELEE_PC_FIGURE_CATCH_DIAGNOSTIC_H
/* Optional controller-only experiment. All fighter/item state is read-only. */
static void MeleePc_FigureCatchInput(HSD_GObj* self,PADStatus* input,unsigned frame)
{
    static unsigned attack_frame;
    static int hit_observed;
    Fighter* fp=self->user_data;
    Item* trophy=NULL;
    HSD_GObj* object=NULL;
    memset(input,0,sizeof(*input));
    for(HSD_GObj* g=HSD_GObjPLinkHead[HSD_GOBJ_PLINK_ITEM];g;g=g->next){
        Item* item=g->user_data;
        if(item && item->kind==It_Kind_Coin && item->xDD4_itemVar.coin.x4==1){
            trophy=item;object=g;break;
        }
    }
    if(!trophy || frame<180 || frame>600)return;
    if(trophy->msid==3)hit_observed=1;
    if(!hit_observed){
        float outside=trophy->pos.x>=0?1.0f:-1.0f;
        /* Stay inside the basket platform: outside approach reached teeter. */
        float goal=trophy->pos.x-outside*12.0f;
        float dx=goal-fp->cur_pos.x,dy=trophy->pos.y-fp->cur_pos.y;
        if(dx>3.0f)input->stickX=45;
        else if(dx< -3.0f)input->stickX=-45;
        else if(fp->facing_dir*outside<0)input->stickX=(s8)(outside*35.0f);
        else if(ftLib_GetGroundAir(self)==GA_Ground && dy> -8.0f && dy<16.0f &&
                ftLib_GetMotionId(self)==ftCo_MS_Wait && !attack_frame){
            attack_frame=frame;
        }
        if(attack_frame && frame-attack_frame<4){input->stickX=0;input->button=PAD_BUTTON_A;}
    }
    if(frame%5==0 || (attack_frame && frame-attack_frame<=8))
        OSReport("NATIVE_FIGURE_APPROACH frame=%u item=%p target_x=%f target_y=%f self_x=%f self_y=%f facing=%f motion=%d item_motion=%d requested_x=%d requested_button=%u attack_frame=%u observed_hit=%d\n",
                 frame,object,trophy->pos.x,trophy->pos.y,fp->cur_pos.x,fp->cur_pos.y,
                 fp->facing_dir,fp->motion_id,trophy->msid,input->stickX,input->button,attack_frame,hit_observed);
}
#endif
