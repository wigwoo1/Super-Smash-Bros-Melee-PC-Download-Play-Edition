#include "video_slots.h"
#include <string.h>
static int find(MeleePc_VideoSlots* q, uint64_t generation, uint64_t id)
{
    if(!q || !q->vi || generation != q->generation || !id) return -1;
    for(int i=0;i<q->vi->nb_xfb;i++)
        if(q->slots[i].active && q->slots[i].id==id) return i;
    return -1;
}
static int oldest(MeleePc_VideoSlots* q)
{
    int result=-1;
    for(int i=0;i<q->vi->nb_xfb;i++)
        if(q->slots[i].active && (result<0 || q->slots[i].id<q->slots[result].id)) result=i;
    return result;
}
static void reconcile(MeleePc_VideoSlots* q)
{
    if(q->failed) return;
    int head=oldest(q);
    if(head<0) return;
    /* Hold out-of-order completion until all earlier submissions complete. */
    uint64_t after=0;
    for(;;) {
        int next=-1;
        for(int i=0;i<q->vi->nb_xfb;i++)
            if(q->slots[i].active && q->slots[i].id>after &&
               (next<0 || q->slots[i].id<q->slots[next].id)) next=i;
        if(next<0 || !q->slots[next].completed) break;
        q->vi->xfb[next].status=next==head ? HSD_VI_XFB_NEXT : HSD_VI_XFB_DRAWDONE;
        after=q->slots[next].id;
    }
    while((head=oldest(q))>=0 && q->slots[head].completed && q->slots[head].presented) {
        for(int i=0;i<q->vi->nb_xfb;i++)
            if(q->vi->xfb[i].status==HSD_VI_XFB_DISPLAY) q->vi->xfb[i].status=HSD_VI_XFB_FREE;
        q->vi->xfb[head].status=HSD_VI_XFB_DISPLAY;
        memset(&q->slots[head],0,sizeof(q->slots[head]));
        q->promoted++;
        int next=oldest(q);
        if(next>=0 && q->slots[next].completed) q->vi->xfb[next].status=HSD_VI_XFB_NEXT;
    }
}
int MeleePc_VideoSlotsInit(MeleePc_VideoSlots* q,HSD_VIInfo* vi,uint64_t generation,
                          void* const* identities,unsigned count)
{
    if(!q || q->vi || !vi || !generation || !identities || count<2 || count>3) return 0;
    for(unsigned i=0;i<count;i++) {
        if(!identities[i]) return 0;
        for(unsigned j=0;j<i;j++) if(identities[i]==identities[j]) return 0;
    }
    memset(q,0,sizeof(*q));q->vi=vi;q->generation=generation;
    vi->nb_xfb=(int)count;
    for(unsigned i=0;i<3;i++) {
        vi->xfb[i].buffer=i<count?identities[i]:0;
        vi->xfb[i].status=i>=count?HSD_VI_XFB_NOUSE:(i?HSD_VI_XFB_FREE:HSD_VI_XFB_DISPLAY);
    }
    vi->xfb[0].vi_all=vi->current;
    return 1;
}
int MeleePc_VideoSlotAcquire(MeleePc_VideoSlots* q,uint64_t generation,uint64_t id)
{
    if(!q || !q->vi || q->failed || generation!=q->generation || id<=q->last_id) return -1;
    for(int i=0;i<q->vi->nb_xfb;i++) if(q->vi->xfb[i].status==HSD_VI_XFB_FREE) {
        q->slots[i]=(MeleePc_VideoSlot){id,1,0,0,0};q->last_id=id;
        q->vi->xfb[i].status=HSD_VI_XFB_DRAWING;return i;
    }
    return -1;
}
int MeleePc_VideoSlotsAvailable(const MeleePc_VideoSlots* q,uint64_t generation)
{
    if(!q || !q->vi || q->failed || generation!=q->generation) return 0;
    for(int i=0;i<q->vi->nb_xfb;i++)
        if(q->vi->xfb[i].status==HSD_VI_XFB_FREE) return 1;
    return 0;
}
int MeleePc_VideoSlotReleaseUnencoded(MeleePc_VideoSlots* q,uint64_t generation,uint64_t id)
{
    int i=find(q,generation,id);
    if(i<0 || q->failed || q->slots[i].submitted ||
       q->vi->xfb[i].status!=HSD_VI_XFB_DRAWING) return 0;
    memset(&q->slots[i],0,sizeof(q->slots[i]));
    q->vi->xfb[i].status=HSD_VI_XFB_FREE;
    return 1;
}
int MeleePc_VideoSlotSubmit(MeleePc_VideoSlots* q,uint64_t generation,uint64_t id)
{
    int i=find(q,generation,id);
    if(i<0 || q->failed || q->slots[i].submitted) return 0;
    for(int j=0;j<q->vi->nb_xfb;j++)
        if(q->slots[j].active && !q->slots[j].submitted && q->slots[j].id<id) return 0;
    q->slots[i].submitted=1;q->vi->xfb[i].status=HSD_VI_XFB_WAITDONE;
    q->vi->xfb[i].vi_all=q->vi->current;return 1;
}
int MeleePc_VideoSlotAck(MeleePc_VideoSlots* q,uint64_t generation,uint64_t id,
                        unsigned kind,unsigned status)
{
    int i=find(q,generation,id);
    if(i<0 || !q->slots[i].submitted || status>1 || (kind!=1 && kind!=2)) return 0;
    unsigned* signal=kind==1?&q->slots[i].completed:&q->slots[i].presented;
    if(*signal) return 0;
    *signal=1;q->failed|=status;reconcile(q);return 1;
}
