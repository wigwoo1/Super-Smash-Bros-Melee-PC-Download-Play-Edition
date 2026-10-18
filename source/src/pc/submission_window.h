#ifndef MELEE_PC_SUBMISSION_WINDOW_H
#define MELEE_PC_SUBMISSION_WINDOW_H
#include <pc/input_submission.h>
#define MELEE_PC_SUBMISSION_WINDOW_SIZE 16
typedef struct MeleePc_SubmissionWindow {
    uint64_t session,next_tick;
    unsigned char owners[4];
    MeleePc_InputGather slots[MELEE_PC_SUBMISSION_WINDOW_SIZE];
} MeleePc_SubmissionWindow;
static void MeleePc_SubmissionWindowEnd(MeleePc_SubmissionWindow* w)
{if(w)memset(w,0,sizeof(*w));}
static int MeleePc_SubmissionWindowBegin(MeleePc_SubmissionWindow* w,uint64_t session,
    uint32_t first,const unsigned char owners[4])
{
    MeleePc_InputGather check={0};
    if(!w||w->session||!MeleePc_InputGatherBegin(&check,session,first,owners))return 0;
    MeleePc_SubmissionWindowEnd(w);w->session=session;w->next_tick=first;
    memcpy(w->owners,owners,4);return 1;
}
static int MeleePc_SubmissionWindowReceive(MeleePc_SubmissionWindow* w,unsigned peer,
    const unsigned char* data,unsigned size)
{
    uint32_t tick;unsigned slot;MeleePc_InputGather candidate;int result;
    if(!w||!w->session||w->next_tick>UINT32_MAX||!data||
       size!=MELEE_PC_INPUT_SUBMISSION_SIZE)return 0;
    tick=(uint32_t)MeleePc_InputGet(data+16,4);
    if(tick<w->next_tick||(uint64_t)tick-w->next_tick>=MELEE_PC_SUBMISSION_WINDOW_SIZE)return 0;
    slot=tick%MELEE_PC_SUBMISSION_WINDOW_SIZE;candidate=w->slots[slot];
    if(!candidate.session&&!MeleePc_InputGatherBegin(&candidate,w->session,tick,w->owners))return 0;
    result=MeleePc_GatherInputSubmission(&candidate,peer,data,size);
    if(result)w->slots[slot]=candidate;return result;
}
static MeleePc_InputGather* MeleePc_SubmissionWindowPeek(MeleePc_SubmissionWindow* w)
{
    MeleePc_InputGather* g;
    if(!w||!w->session||w->next_tick>UINT32_MAX)return NULL;
    g=&w->slots[w->next_tick%MELEE_PC_SUBMISSION_WINDOW_SIZE];
    return g->next_tick==w->next_tick&&MeleePc_InputGatherReady(g)?g:NULL;
}
/* Controller staging advances the gather. Drop only after successful staging
 * or outgoing queue publication; never advance past an incomplete tick. */
static int MeleePc_SubmissionWindowDrop(MeleePc_SubmissionWindow* w)
{
    MeleePc_InputGather* g;
    if(!w||!w->session||w->next_tick>UINT32_MAX)return 0;
    g=&w->slots[w->next_tick%MELEE_PC_SUBMISSION_WINDOW_SIZE];
    if(g->session!=w->session||!(g->next_tick==w->next_tick+1||
       (g->next_tick==w->next_tick&&MeleePc_InputGatherReady(g))))return 0;
    MeleePc_InputGatherEnd(g);++w->next_tick;return 1;
}
#endif
