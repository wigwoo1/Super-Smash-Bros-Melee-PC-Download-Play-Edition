#ifndef MELEE_PC_ITEM_ARCHIVE_RELOCATIONS_H
#define MELEE_PC_ITEM_ARCHIVE_RELOCATIONS_H
/* Archive Locate consumes every relocation, including unused descriptor
 * aliases. The relocation table proves pointer storage, not target type.
 * Run after typed planning; conflicts with byte/halfword payloads refuse.
 * This does not establish scalar coverage of unvisited objects. */
static void MeleePc_ItemArchiveRelocationsPlan(MeleePc_TitlePlan* p){
 if(p->error)return;
 for(uint32_t i=0;i<(p->size+3)/4 && !p->error;i++){
  if(p->relocs[i])title_pointer(p,i*4);
 }
}
#endif
