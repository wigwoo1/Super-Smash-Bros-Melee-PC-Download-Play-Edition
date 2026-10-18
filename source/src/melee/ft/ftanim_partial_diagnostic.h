/* Bounded actual-call inventory only. Never changes selection or hides a bad
 * context. Source/model cross-products are not assumed to be reachable. */
#ifndef MELEE_FT_PARTIAL_DIAGNOSTIC_H
#define MELEE_FT_PARTIAL_DIAGNOSTIC_H
#ifdef TARGET_PC
#include <stdlib.h>
#include <dolphin/os.h>
extern unsigned int HSD_JObjDiagnosticMatchFrame;
static void FtPartialDiagnostic(Fighter* fp,int part,int blending)
{
    static int enabled=-1;static unsigned count;unsigned source,source_parts=0,actor_parts=0,present=0;
    if(enabled<0)enabled=getenv("MELEE_DIAGNOSTIC_GAME_STARTUP")&&getenv("MELEE_DIAGNOSTIC_PARTIAL_ANIMATION");
    if(!enabled||HSD_JObjDiagnosticMatchFrame<300||count>=64)return;
    source=(unsigned)fp->x594.x594_s32&63;
    if(source<34&&ftPartsTable[source])source_parts=ftPartsTable[source]->parts_num;
    if((unsigned)fp->kind<34&&ftPartsTable[fp->kind])actor_parts=ftPartsTable[fp->kind]->parts_num;
    if(part>=0&&(unsigned)part<actor_parts&&fp->parts)present=fp->parts[part].x8.x0.flags_b1;
    ++count;
    OSReport("NATIVE_PARTIAL_ANIMATION sample=%u frame=%u actor=%d animation=%d source=%u start=%d mask=%u blending=%d source_parts=%u actor_parts=%u root_present=%u outside_source_extent=%d flags=%08x\n",
        count,HSD_JObjDiagnosticMatchFrame,fp->kind,fp->anim_id,source,part,((unsigned)fp->x594.x594_s32>>9)&8191,blending,source_parts,actor_parts,present,part<0||(unsigned)part>=source_parts,(unsigned)fp->x594.x594_s32);
}
#endif
#endif
