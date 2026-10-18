#ifndef MELEE_PC_STAGE_SHRINE_PARAMETERS_H
#define MELEE_PC_STAGE_SHRINE_PARAMETERS_H
/* grshrine.c only assigns the opaque pointer; it never reads its payload.
 * Validate the audited zero root word, preserve all unused neighboring bytes.
 * Do not infer a scalar structure from adjacent archive storage. */
static void MeleePc_StageShrineParametersPlan(MeleePc_TitlePlan* p,uint32_t root){
 if(p->error)return;
 if((root&3) || !title_range(p,root,4)){p->error=-1;return;}
 if(p->relocs[root/4] || (p->external_slots && p->external_slots[root/4]) || title_u32(p,root)){p->error=-2;return;}
 title_claim(p,root,4,1);
}
#endif
