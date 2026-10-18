#ifndef MELEE_PC_COSTUME_SHEIK_NR_PLAN_H
#define MELEE_PC_COSTUME_SHEIK_NR_PLAN_H
/* Sheik's source-registered normal costume exports a JObj tree and a
 * separate material-animation joint root. Both are required by ftSk setup. */
static void MeleePc_CostumeSheikNrPlan(MeleePc_TitlePlan* p) {
    if (p->error) return;
    if (p->size != 374576) { p->error = -2; return; }
    title_enqueue(p, 110024, TITLE_HSD_Joint);
    title_enqueue(p, 276272, TITLE_HSD_MatAnimJoint);
}
#endif
