#ifndef MELEE_PC_FT_ACTION_FIELDS_H
#define MELEE_PC_FT_ACTION_FIELDS_H
#ifdef TARGET_PC
#include <stdint.h>
#include <string.h>
static uint32_t MeleePc_FtActionWord(const void* data){uint32_t v;memcpy(&v,data,4);return v;}
#define FT_ACTION_U(cmd,member,shift,mask) ((MeleePc_FtActionWord((cmd)->x8.u)>>(shift))&(mask))
#else
#define FT_ACTION_U(cmd,member,shift,mask) ((cmd)->x8.u->member)
#endif
#define FT_ACTION_S16(cmd,member,shift) ((s16)FT_ACTION_U(cmd,member,shift,0xffffU))
#endif
