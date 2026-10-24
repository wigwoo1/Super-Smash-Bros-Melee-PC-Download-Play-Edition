#include "archive.h"

#include <string.h>

#include <dolphin/os.h>
#ifdef TARGET_PC
#include <pc/archive_native.h>
#include <pc/archive_layout.h>
#include <pc/asset_regions.h>
#include <pc/diagnostic_costume_snapshot.h>

/* Emit bounded archive identity when the native schema decoder refuses a
 * body. File size alone is not enough to identify a runtime asset: different
 * files can have similar sizes. This diagnostic reads only the declared HSD
 * tables and checks every extent before looking at symbol strings. */
static void MeleePc_LogUnsupportedArchive(const u8* src, size_t file_size)
{
    size_t cursor = sizeof(HSD_ArchiveHeader);
    size_t reloc_offset, public_offset, extern_offset, symbol_offset;
    u32 body_size, reloc_count, public_count, extern_count, i;

    if (src == NULL || file_size < sizeof(HSD_ArchiveHeader)) {
        return;
    }
    body_size = MeleePc_DatBe32(src + 4);
    reloc_count = MeleePc_DatBe32(src + 8);
    public_count = MeleePc_DatBe32(src + 12);
    extern_count = MeleePc_DatBe32(src + 16);
    if (body_size > file_size - cursor) {
        return;
    }
    cursor += body_size;
    reloc_offset = cursor;
    /* HSD relocation slots are u32 offsets, not the 8-byte public/extern
     * records below. Keep this read-only diagnostic aligned with DAT layout. */
    if (reloc_count > (file_size - cursor) / 4) {
        return;
    }
    cursor += (size_t) reloc_count * 4;
    public_offset = cursor;
    if (public_count > (file_size - cursor) / 8) {
        return;
    }
    cursor += (size_t) public_count * 8;
    if (extern_count > (file_size - cursor) / 8) {
        return;
    }
    extern_offset = cursor;
    cursor += (size_t) extern_count * 8;
    symbol_offset = cursor;

    OSReport("NATIVE_ARCHIVE unsupported_meta bytes=%u body=%u reloc=%u "
             "public=%u extern=%u reloc_offset=%u roots=",
             (u32) file_size, body_size, reloc_count, public_count,
             extern_count, (u32) reloc_offset);
    for (i = 0; i < public_count && i < 32; ++i) {
        size_t entry = public_offset + (size_t) i * 8;
        u32 symbol = MeleePc_DatBe32(src + entry + 4);
        size_t remaining = file_size - symbol_offset;
        const char* name;

        if (symbol >= remaining) {
            OSReport("[%u]=<out-of-range:%u> ", i, symbol);
            continue;
        }
        name = (const char*) src + symbol_offset + symbol;
        if (memchr(name, 0, remaining - symbol) == NULL) {
            OSReport("[%u]=<unterminated:%u> ", i, symbol);
        } else {
            OSReport("[%u]=%u:%s ", i, MeleePc_DatBe32(src + entry),
                     name);
            if (strcmp(name, "ftDataKirbyCopyDrmario") == 0) {
                u32 root = MeleePc_DatBe32(src + entry);
                u32 word;
                if ((root & 3) || root > body_size || body_size - root < 16) {
                    OSReport("KIRBY_DR_ROOT_OUT_OF_RANGE root=%u ", root);
                } else {
                    OSReport("KIRBY_DR_WORDS root=%u:", root);
                    for (word = 0; word < 4; ++word) {
                        OSReport(" %u=%08x", word,
                                 MeleePc_DatBe32(src + sizeof(HSD_ArchiveHeader) +
                                                 root + word * 4));
                    }
                }
                OSReport(" KIRBY_DR_TAIL:");
                if (body_size >= 64) {
                    u32 tail;
                    for (tail = body_size - 64; tail <= body_size - 4;
                         tail += 4) {
                        OSReport(" %u=%08x", tail,
                                 MeleePc_DatBe32(src + sizeof(HSD_ArchiveHeader) +
                                                 tail));
                    }
                }
                OSReport(" KIRBY_DR_RELOCS:");
                for (word = 0; word < reloc_count; ++word) {
                    u32 at = MeleePc_DatBe32(src + reloc_offset + word * 4);
                    if ((at >= root && at < root + 16) ||
                        (body_size >= 64 && at >= body_size - 64)) {
                        OSReport(" %u", at);
                    }
                }
                OSReport(" ");
            }
        }
    }
    OSReport("imports=");
    for (i = 0; i < extern_count && i < 8; ++i) {
        size_t entry = extern_offset + (size_t) i * 8;
        u32 symbol = MeleePc_DatBe32(src + entry + 4);
        size_t remaining = file_size - symbol_offset;
        const char* name;

        if (symbol >= remaining) {
            OSReport("[%u]=<out-of-range:%u> ", i, symbol);
            continue;
        }
        name = (const char*) src + symbol_offset + symbol;
        if (memchr(name, 0, remaining - symbol) == NULL) {
            OSReport("[%u]=<unterminated:%u> ", i, symbol);
        } else {
            OSReport("[%u]=%u:%s ", i, MeleePc_DatBe32(src + entry),
                     name);
        }
    }
    OSReport("\n");
}
#endif

static inline void Locate(HSD_Archive* archive)
{
    u32 i;
    u32* ptr;

    for (i = 0; i < archive->header.nb_reloc; i++) {
        ptr = (u32*) (archive->data + archive->reloc_info[i].offset);
        *ptr += (uintptr_t) archive->data;
    }
}

s32 HSD_ArchiveParse(HSD_Archive* archive, u8* src, size_t file_size)
{
    u32 offset;

    if (archive == NULL || src == NULL || file_size < sizeof(HSD_ArchiveHeader)) {
        return -1;
    }
#ifdef TARGET_PC
    if(MeleePc_AssetAllocationRegistered(src))return -1;
    if(getenv("MELEE_DIAGNOSTIC_GAME_STARTUP"))
        OSReport("NATIVE_ARCHIVE parse_begin bytes=%u\n",(u32)file_size);
    u32 native_size;memcpy(&native_size,src,4);
    unsigned char* native_original = NULL;
    u32 native_original_size = 0;
    /* Exact Sheik fighter/copy archive envelopes. Preserve before conversion; the
     * decoder still owns full symbol/layout validation and may refuse it. */
    if(native_size!=file_size &&
       ((file_size==166907 && MeleePc_DatBe32(src+4)==156352 &&
         MeleePc_DatBe32(src+8)==2626 && MeleePc_DatBe32(src+12)==1 &&
         MeleePc_DatBe32(src+16)==0) ||
        MeleePc_KirbyCopySheikProfile(src,file_size))){
        native_original_size=MeleePc_DatBe32(src+4);
        native_original=malloc(native_original_size);
        if(!native_original)return -1;
        memcpy(native_original,src+32,native_original_size);
    }
    /* Every admitted Sheik costume retains its packed payload view; graph
     * conversion still validates before mutation and owns refusal cleanup. */
    if(!native_original && native_size!=file_size){
        const MeleePcSheikCostumeProfile* profile=MeleePc_SheikCostumeProfile(src,file_size);
        if(profile){
            native_original_size=profile->body;
            native_original=malloc(native_original_size);
            if(!native_original)return -1;
            memcpy(native_original,src+32,native_original_size);
        }
    }
    if(!native_original && native_size!=file_size &&
       getenv("MELEE_DIAGNOSTIC_GAME_STARTUP") &&
       getenv("MELEE_DIAGNOSTIC_ORIGINAL_MESH")){
        native_original_size=MeleePc_DiagnosticCostumeBody(src,file_size);
        if(native_original_size){
            native_original=malloc(native_original_size);
            if(!native_original)return -1;
            memcpy(native_original,src+32,native_original_size);
        }
    }
    if(native_size!=file_size){
        int result=MeleePc_DatDecode(src,file_size);
        if(result){
            if (getenv("MELEE_DIAGNOSTIC_GAME_STARTUP")) {
                OSReport("NATIVE_ARCHIVE decode_result=%d bytes=%u body=%u "
                         "reloc=%u public=%u extern=%u\n",
                         result, (u32) file_size, MeleePc_DatBe32(src + 4),
                         MeleePc_DatBe32(src + 8), MeleePc_DatBe32(src + 12),
                         MeleePc_DatBe32(src + 16));
                if (result == -2) {
                    MeleePc_LogUnsupportedArchive(src, file_size);
                }
            }
            OSReport("Native archive conversion refused: %s, bytes=%u\n",
                     result==-2 ? "unsupported typed body schema" : "invalid bounded layout",(u32)file_size);
            free(native_original);return -1;
        }
    }
    if(!MeleePc_ArchiveLayout(src,file_size)){free(native_original);return -1;}
    u32 body_size=MeleePc_ArchiveWord(src+4);
    if(body_size && !MeleePc_RegisterOwnedAssetRegion(src,src+32,body_size)) {
        OSReport("Native archive registration refused: registry full or overlapping body\n");
        free(native_original);return -1;
    }
    if(native_original){
        int preserved=MeleePc_PreserveAssetBytes(src,src+32,native_original,native_original_size);
        free(native_original);
        if(!preserved){MeleePc_UnregisterAssetAllocation(src);return -1;}
        if(getenv("MELEE_DIAGNOSTIC_GAME_STARTUP"))
            OSReport("NATIVE_ARCHIVE original_bytes_preserved bytes=%u body=%u owner_registered=1\n",
                     (u32)file_size,body_size);
    }
#endif

    memset(archive, 0, sizeof(HSD_Archive));
    archive->flags |= 1;
    memcpy(archive, src, sizeof(HSD_ArchiveHeader));

    if (archive->header.file_size != file_size) {
        OSReport("HSD_ArchiveParse: byte-order mismatch! Please check data "
                 "format %x %x\n",
                 archive->header.file_size, file_size);
        return -1;
    }

    offset = sizeof(HSD_ArchiveHeader);
    if (archive->header.data_size != 0) { // Body Size
        archive->data = src + sizeof(HSD_ArchiveHeader);
        offset = archive->header.data_size + sizeof(HSD_ArchiveHeader);
    }
    if (archive->header.nb_reloc != 0) { // Relocation Size
        archive->reloc_info = (HSD_ArchiveRelocationInfo*) (src + offset);
        offset = offset +
                 archive->header.nb_reloc * sizeof(HSD_ArchiveRelocationInfo);
    }
    if (archive->header.nb_public != 0) { // Root Size
        archive->public_info = (HSD_ArchivePublicInfo*) (src + offset);
        offset =
            offset + archive->header.nb_public * sizeof(HSD_ArchivePublicInfo);
    }
    if (archive->header.nb_extern != 0) { // XRef Size
        archive->extern_info = (HSD_ArchiveExternInfo*) (src + offset);
        offset =
            offset + archive->header.nb_extern * sizeof(HSD_ArchiveExternInfo);
    }
    if (offset < archive->header.file_size) { // File Size
        archive->symbols = (char*) (src + offset);
    }

    archive->top_ptr = src;
    Locate(archive);
#ifdef TARGET_PC
    if(getenv("MELEE_DIAGNOSTIC_GAME_STARTUP"))
        OSReport("NATIVE_ARCHIVE parse_end bytes=%u reloc=%u first_public=%s\n",
                 (u32)file_size,archive->header.nb_reloc,
                 archive->header.nb_public ? archive->symbols+archive->public_info[0].symbol : "<none>");
    if (getenv("MELEE_DIAGNOSTIC_GAME_STARTUP") &&
        HSD_ArchiveGetPublicAddress(archive, "ftDataKoopa")) {
        OSReport("Native Koopa archive parse accepted: bytes=%u reloc=%u extern=%u\n",
                 (u32)file_size, archive->header.nb_reloc,
                 archive->header.nb_extern);
    }
    if (getenv("MELEE_DIAGNOSTIC_GAME_STARTUP") &&
        HSD_ArchiveGetPublicAddress(archive, "ftDataLink")) {
        OSReport("Native Link archive parse accepted: bytes=%u reloc=%u extern=%u\n",
                 (u32)file_size, archive->header.nb_reloc,
                 archive->header.nb_extern);
    }
    if (getenv("MELEE_DIAGNOSTIC_GAME_STARTUP") &&
        HSD_ArchiveGetPublicAddress(archive, "ftDataKirby")) {
        OSReport("Native Kirby archive parse accepted: bytes=%u reloc=%u extern=%u\n",
                 (u32)file_size, archive->header.nb_reloc,
                 archive->header.nb_extern);
    }
    if (getenv("MELEE_DIAGNOSTIC_GAME_STARTUP") &&
        HSD_ArchiveGetPublicAddress(archive, "SIS_GrPStadiumData")) {
        OSReport("Native Stadium archive parse accepted: bytes=%u public=%u extern=%u\n",
                 (u32)file_size, archive->header.nb_public,
                 archive->header.nb_extern);
    }
#endif

    return 0;
}

void* HSD_ArchiveGetPublicAddress(HSD_Archive* archive, const char* symbols)
{
    u32 i;

    for (i = 0; i < archive->header.nb_public; i++) {
        int comparison =
            strcmp(archive->symbols + archive->public_info[i].symbol, symbols);

        if (comparison == 0) {
            // If both strings are equal, we've found the node
            return archive->data + archive->public_info[i].offset;
        }
    }

    return NULL;
}

char* HSD_ArchiveGetExtern(HSD_Archive* archive, int index)
{
    if (index < 0 || archive->header.nb_extern <= (u32) index) {
        return NULL;
    }

    return archive->symbols + archive->extern_info[index].symbol;
}

void HSD_ArchiveLocateExtern(HSD_Archive* archive, const char* symbols,
                             void* addr)
{
    u32 offset = -1U;
    u32 i;

    for (i = 0; i < archive->header.nb_extern; i++) {
        int comparison =
            strcmp(symbols, archive->symbols + archive->extern_info[i].symbol);

        if (comparison == 0) {
            offset = archive->extern_info[i].offset;
            break;
        }
    }

    if (offset == -1U) {
        return;
    }

    while (offset != -1U && offset < archive->header.data_size) {
        u32* slot = (u32*) (archive->data + offset);
        u32 next = *slot;
        *slot = (uintptr_t) addr;
        offset = next;
    }
}
