#ifndef MELEE_PC_FRAME_INPUT_H
#define MELEE_PC_FRAME_INPUT_H
#include <dolphin/pad.h>
#include <string.h>
#include <stdint.h>
/* Owner-thread transaction: all ports validate before any output changes. */
static int MeleePc_StageFourPortInput(PADStatus output[4],const PADStatus input[4],unsigned pending_mask)
{
    PADStatus candidate[4];
    if(!output || !input || pending_mask)return 0;
    for(unsigned i=0;i<4;i++){
        if(input[i].err!=PAD_ERR_NONE && input[i].err!=PAD_ERR_NO_CONTROLLER)return 0;
        candidate[i]=input[i];
    }
    memcpy(output,candidate,sizeof(candidate));return 1;
}
typedef struct MeleePc_FrameInputOrder {
    uint64_t next_frame;
    int active;
    int pending;
} MeleePc_FrameInputOrder;
static void MeleePc_BeginFrameInputOrder(MeleePc_FrameInputOrder* order,uint32_t first_frame)
{
    if(order){order->next_frame=first_frame;order->active=1;order->pending=0;}
}
static int MeleePc_StageOrderedFrameInput(MeleePc_FrameInputOrder* order,uint32_t frame,
    PADStatus output[4],const PADStatus input[4])
{
    if(!order || !order->active || order->pending || order->next_frame!=frame)return 0;
    if(!MeleePc_StageFourPortInput(output,input,0))return 0;
    order->pending=1;return 1;
}
/* Call only after all four values reach the controller consumer. */
static int MeleePc_CompleteFrameInput(MeleePc_FrameInputOrder* order)
{
    if(!order || !order->active || !order->pending)return 0;
    order->pending=0;
    if(++order->next_frame>UINT32_MAX)order->active=0;
    return 1;
}
static void MeleePc_EndFrameInputOrder(MeleePc_FrameInputOrder* order)
{
    if(order){order->active=0;order->pending=0;order->next_frame=0;}
}
#endif
