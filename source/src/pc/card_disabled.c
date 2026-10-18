/* Session-only PC build: no card devices, filesystem access or queued I/O.
 * Async requests fail immediately, so their callbacks are never scheduled. */
#include <dolphin/card.h>

#define NO_CARD(signature) signature { return CARD_RESULT_NOCARD; }

int CARDProbe(s32 chan) { return 0; }
s32 CARDGetXferredBytes(s32 chan) { return 0; }
NO_CARD(s32 CARDGetResultCode(s32 chan))
NO_CARD(s32 CARDProbeEx(s32 chan, s32* memSize, s32* sectorSize))
NO_CARD(s32 CARDCheckAsync(s32 chan, CARDCallback callback))
NO_CARD(s32 CARDMountAsync(s32 chan, void* workArea, CARDCallback detachCallback, CARDCallback attachCallback))
NO_CARD(s32 CARDUnmount(s32 chan))
NO_CARD(s32 CARDFreeBlocks(s32 chan, s32* byteNotUsed, s32* filesNotUsed))
NO_CARD(s32 CARDFastOpen(s32 chan, s32 fileNo, CARDFileInfo* fileInfo))
NO_CARD(s32 CARDOpen(s32 chan, const char* fileName, CARDFileInfo* fileInfo))
NO_CARD(s32 CARDClose(CARDFileInfo* fileInfo))
NO_CARD(s32 CARDFormat(s32 chan))
NO_CARD(s32 CARDCreateAsync(s32 chan, const char* fileName, u32 size, CARDFileInfo* fileInfo, CARDCallback callback))
NO_CARD(s32 CARDDeleteAsync(s32 chan, const char* fileName, CARDCallback callback))
NO_CARD(s32 CARDRenameAsync(s32 chan, const char* oldName, const char* newName, CARDCallback callback))
NO_CARD(s32 CARDGetStatus(s32 chan, s32 fileNo, CARDStat* stat))
NO_CARD(s32 CARDSetStatusAsync(s32 chan, s32 fileNo, const CARDStat* stat, CARDCallback callback))
NO_CARD(s32 CARDRead(const CARDFileInfo* fileInfo, void* addr, s32 length, s32 offset))
NO_CARD(s32 CARDReadAsync(const CARDFileInfo* fileInfo, void* addr, s32 length, s32 offset, CARDCallback callback))
NO_CARD(s32 CARDWrite(const CARDFileInfo* fileInfo, const void* addr, s32 length, s32 offset))
NO_CARD(s32 CARDWriteAsync(const CARDFileInfo* fileInfo, const void* addr, s32 length, s32 offset, CARDCallback callback))
