#ifndef MELEE_PC_FIGHTER_COMMAND_DISPATCH_H
#define MELEE_PC_FIGHTER_COMMAND_DISPATCH_H
#ifdef TARGET_PC
/* Fighter animation streams retain PPC packed bytes. Relocated branch words
 * are native pointers and are handled by Command_ExecutePackedBe. */
static inline unsigned MeleePc_FighterCommandOpcode(const void* data){
 return ((const unsigned char*)data)[0]>>2;
}
#define FT_FIGHTER_OPCODE(cmd) MeleePc_FighterCommandOpcode((cmd)->x8.u)
#define FT_FIGHTER_EXECUTE(cmd,id) Command_ExecutePackedBe(cmd,id)
#else
#define FT_FIGHTER_OPCODE(cmd) ((cmd)->x8.u->Command_09.id)
#define FT_FIGHTER_EXECUTE(cmd,id) Command_Execute(cmd,id)
#endif
#endif
