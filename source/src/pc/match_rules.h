#ifndef MELEE_PC_MATCH_RULES_H
#define MELEE_PC_MATCH_RULES_H
#include <melee/mn/types.h>
#include <pc/input_packet.h>
#define MELEE_PC_RULES_CAPACITY 2048
/* Canonical semantic fields, never struct padding or function addresses.
 * Caller must map callbacks to stable registered IDs shared by the build.
 * Unsupported pointer-backed mode payloads fail rather than hash addresses. */
static unsigned MeleePc_EncodeMatchRules(unsigned char* output,unsigned capacity,
    const StartMeleeData* match,const uint32_t callback_ids[7])
{
    unsigned char bytes[MELEE_PC_RULES_CAPACITY]={0};unsigned offset=8;
    const StartMeleeRules* r;uint32_t bits;
    if(!output||!match||!callback_ids)return 0;
    r=&match->rules;if(r->x54||r->x58)return 0;
    memcpy(bytes,"MRUL",4);MeleePc_InputPut(bytes+4,1,4);
#define WORD(v) do {MeleePc_InputPut(bytes+offset,(uint32_t)(v),4);offset+=4;} while(0)
#define FLOAT(v) do {memcpy(&bits,&(v),4);if((bits&0x7f800000u)==0x7f800000u)return 0;WORD(bits);} while(0)
    WORD(r->match_kind);
    WORD(r->x0_3);
    WORD(r->timer_enabled);
    WORD(r->timer_counts_up);
    WORD(r->x1_0);
    WORD(r->x1_1);
    WORD(r->x1_2);
    WORD(r->x1_3);
    WORD(r->x1_4);
    WORD(r->x1_5);
    WORD(r->timer_shows_hours);
    WORD(r->friendly_fire);
    WORD(r->is_stock);
    WORD(r->x2_1);
    WORD(r->x2_2);
    WORD(r->single_button);
    WORD(r->disable_pausing);
    WORD(r->x2_5);
    WORD(r->x2_6);
    WORD(r->x2_7);
    WORD(r->x3_0);
    WORD(r->x3_1);
    WORD(r->x3_2);
    WORD(r->x3_3);
    WORD(r->x3_4);
    WORD(r->x3_5);
    WORD(r->x3_6);
    WORD(r->x3_7);
    WORD(r->x4_0);
    WORD(r->is_vs);
    WORD(r->x4_2);
    WORD(r->x4_3);
    WORD(r->x4_4);
    WORD(r->x4_5);
    WORD(r->x4_6);
    WORD(r->x4_7);
    WORD(r->x5_0);
    WORD(r->x5_1);
    WORD(r->x5_2);
    WORD(r->x5_3);
    WORD(r->x5_4);
    WORD(r->x5_5);
    WORD(r->x5_6);
    WORD(r->x5_7);
    WORD(r->x6);
    WORD(r->x7);
    WORD(r->is_teams);
    WORD(r->x9);
    WORD(r->xA);
    WORD(r->item_freq);
    WORD(r->sd_penalty);
    WORD(r->xD);
    WORD(r->stkind);
    WORD(r->time_limit);
    WORD(r->x14);
    WORD(r->x18);
    WORD(r->x28);
    WORD(r->x20);WORD(r->x20>>32);
    FLOAT(r->x2C);
    FLOAT(r->x30);
    FLOAT(r->game_speed);
    if((r->on_unpause_override!=NULL)!=(callback_ids[0]!=0))return 0;WORD(callback_ids[0]);
    if((r->on_pause_override!=NULL)!=(callback_ids[1]!=0))return 0;WORD(callback_ids[1]);
    if((r->check_for_pauser_override!=NULL)!=(callback_ids[2]!=0))return 0;WORD(callback_ids[2]);
    if((r->on_match_start!=NULL)!=(callback_ids[3]!=0))return 0;WORD(callback_ids[3]);
    if((r->on_frame_start!=NULL)!=(callback_ids[4]!=0))return 0;WORD(callback_ids[4]);
    if((r->on_frame_end!=NULL)!=(callback_ids[5]!=0))return 0;WORD(callback_ids[5]);
    if((r->on_match_end!=NULL)!=(callback_ids[6]!=0))return 0;WORD(callback_ids[6]);
    WORD(GM_MAX_PLAYERS);
    for(unsigned i=0;i<GM_MAX_PLAYERS;i++){
        const PlayerInitData* p=&match->players[i];
        WORD(p->ckind);
        WORD(p->slot_type);
        WORD(p->stocks);
        WORD(p->color);
        WORD(p->slot);
        WORD(p->spawn_pos);
        WORD(p->spawn_dir);
        WORD(p->sub_color);
        WORD(p->handicap);
        WORD(p->team);
        WORD(p->nametag);
        WORD(p->xB);
        WORD(p->rumble_enabled);
        WORD(p->xC_b1);
        WORD(p->vs_metal);
        WORD(p->xC_b3);
        WORD(p->vs_invisible);
        WORD(p->xC_b5);
        WORD(p->xC_b6);
        WORD(p->xC_b7);
        WORD(p->xD_b0);
        WORD(p->xD_b1);
        WORD(p->xD_b2);
        WORD(p->xD_b3);
        WORD(p->xD_b4);
        WORD(p->xD_b5);
        WORD(p->xD_b6);
        WORD(p->xD_b7);
        WORD(p->cpu_kind);
        WORD(p->cpu_level);
        WORD(p->damage);
        WORD(p->damage1);
        WORD(p->hp);
        FLOAT(p->attack_ratio);
        FLOAT(p->defense_ratio);
        FLOAT(p->model_scale);
    }
#undef FLOAT
#undef WORD
    if(offset>sizeof(bytes)||capacity<offset)return 0;
    memcpy(output,bytes,offset);return offset;
}
#endif
