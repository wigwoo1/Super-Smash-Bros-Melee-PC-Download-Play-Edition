#ifndef MELEE_PC_ITEM_SOUND_ASSET_DECODE_H
#define MELEE_PC_ITEM_SOUND_ASSET_DECODE_H
#include "asset_regions.h"
#include "item_effect_sound_decode.h"
/* Keep the runtime cursor in the native archive. Only bounded packed reads
 * use immutable originals when an archive owns a pre-conversion snapshot. */
static int MeleePc_ItemSoundAssetDecode(const void* command,MeleePc_ItemSoundArgs* out){
 uint32_t remaining=0;const void* packed=command;
 if(!command||!out||!MeleePc_AssetBytesRemaining(command,&remaining)||remaining<12)return 0;
 const void* original=NULL;
 if(MeleePc_OriginalAssetBytes(command,12,&original))packed=original;
 return MeleePc_ItemSoundDecode(packed,12,out);
}
#endif
