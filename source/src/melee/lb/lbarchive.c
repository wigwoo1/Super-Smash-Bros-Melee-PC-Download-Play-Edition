#include "lbarchive.h"

#include <stdarg.h>
#include <string.h>
#ifdef TARGET_PC
#include <stdlib.h>
#include <windows.h>
#include <pc/asset_regions.h>
#endif

#include "lbdvd.h"
#include "lbfile.h"
#include "lbheap.h"
#include <dolphin/os.h>
#include <sysdolphin/baselib/archive.h>
#include <sysdolphin/baselib/debug.h>

void lbArchive_InitializeDAT(HSD_Archive* archive, void* data, size_t length)
{
    const char* symbol;
    int i = 0;

    if (HSD_ArchiveParse(archive, data, length) == -1) {
        OSReport("HSD_ArchiveParse error!\n");
        HSD_ASSERT(73, 0);
    }

    while (true) {
        symbol = HSD_ArchiveGetExtern(archive, i++);
        if (symbol != NULL) {
            HSD_ArchiveLocateExtern(archive, symbol, NULL);
        }
        if (symbol == NULL) {
            return;
        }
    }
}

static inline void vLoadSections(HSD_Archive* archive, void** symbol,
                                 va_list symbols)
{
    const char* symbol_name;

    for (; symbol != NULL; symbol = va_arg(symbols, void**)) {
        symbol_name = va_arg(symbols, const char*);
        *symbol = NULL;
        *symbol = HSD_ArchiveGetPublicAddress(archive, symbol_name);
        if (*symbol == NULL) {
            OSReport("Cannot find symbol %s.\n", symbol_name);
        }
    }
}

void(lbArchive_LoadSections)(HSD_Archive* archive, void* symbol, ...)
{
    va_list symbols;

    va_start(symbols, symbol);
    vLoadSections(archive, symbol, symbols);
    va_end(symbols);
}

static inline void readArchive(const char* filename, void* data,
                               HSD_Archive* archive)
{
    size_t length;

    lbFile_8001668C(filename, data, &length);
    lbArchive_InitializeDAT(archive, data, length);
}

static inline HSD_Archive* loadArchive(const char* filename)
{
    HSD_Archive* archive;
    void* data;

#ifdef TARGET_PC
    if (getenv("MELEE_DIAGNOSTIC_GAME_STARTUP")) {
        OSReport("NATIVE_ARCHIVE_REQUEST filename=%s\n", filename);
    }
#endif
    data = lbHeap_80015BD0(0, OSRoundUp32B(lbFileGetSize(filename)));
    archive = lbHeap_80015BD0(0, sizeof(HSD_Archive));
    readArchive(filename, data, archive);
    return archive;
}

HSD_Archive* lbArchive_LoadArchive(const char* filename)
{
    return loadArchive(filename);
}

static inline void vLoadSectionsFatal(HSD_Archive* archive, void** symbol,
                                      va_list symbols)
{
    const char* symbol_name;

    for (; symbol != NULL; symbol = va_arg(symbols, void**)) {
        symbol_name = va_arg(symbols, const char*);
        *symbol = NULL;
        *symbol = HSD_ArchiveGetPublicAddress(archive, symbol_name);
        if (*symbol == NULL) {
            OSReport("Cannot find symbol %s.\n", symbol_name);
            HSD_ASSERT(112, 0);
        }
    }
}

HSD_Archive*(lbArchive_LoadSymbols) (const char* filename, void* symbols, ...)
{
    va_list sections;
    HSD_Archive* archive;

#ifdef TARGET_PC
    if (getenv("MELEE_DIAGNOSTIC_GAME_STARTUP")) {
        uintptr_t caller = (uintptr_t) __builtin_return_address(0);
        uintptr_t image_base = (uintptr_t) GetModuleHandleA(NULL);
        OSReport("NATIVE_ARCHIVE_API api=LoadSymbols filename=%s caller_map=%08x\n",
                 filename, (u32) (caller - image_base + 0x00400000));
    }
#endif
    va_start(sections, symbols);

    archive = loadArchive(filename);
    vLoadSectionsFatal(archive, symbols, sections);

    va_end(sections);
    return archive;
}

HSD_Archive*(lbArchive_80016DBC) (const char* filename, void* symbols, ...)
{
    va_list sections;
    HSD_Archive* archive;

#ifdef TARGET_PC
    if (getenv("MELEE_DIAGNOSTIC_GAME_STARTUP")) {
        uintptr_t caller = (uintptr_t) __builtin_return_address(0);
        uintptr_t image_base = (uintptr_t) GetModuleHandleA(NULL);
        OSReport("NATIVE_ARCHIVE_API api=LoadSections filename=%s caller_map=%08x\n",
                 filename, (u32) (caller - image_base + 0x00400000));
    }
#endif
    va_start(sections, symbols);

    archive = loadArchive(filename);
    vLoadSections(archive, symbols, sections);

    va_end(sections);
    return archive;
}

void lbArchive_80016EFC(HSD_Archive* archive)
{
    HSD_ASSERT(0xFC, archive);
    HSD_ASSERT(0xFD, archive->flags & HSD_ARCHIVE_DONT_FREE);
    lbHeap_80015CA8(0, archive->data - sizeof(archive->header));
    lbHeap_80015CA8(0, archive);
}

bool lbArchive_80016F80(HSD_Archive** dst, const char* filename)
{
    HSD_Archive* archive;
    bool preloaded;

    archive = lbDvd_8001819C(filename);
    if (archive != NULL) {
        preloaded = true;
    } else {
        archive = loadArchive(filename);
        preloaded = false;
    }
    if (dst != NULL) {
        *dst = archive;
    }
    return preloaded;
}

bool(lbArchive_80017040)(HSD_Archive** dst, const char* filename,
                         void* symbols, ...)
{
    HSD_Archive* archive;
    bool preloaded;
    va_list args;

    va_start(args, symbols);

    archive = lbDvd_8001819C(filename);
    if (archive != NULL) {
        preloaded = true;
    } else {
        archive = loadArchive(filename);
        preloaded = false;
    }

    vLoadSectionsFatal(archive, symbols, args);

    va_end(args);

    if (dst != NULL) {
        *dst = archive;
    }
    return preloaded;
}

bool(lbArchive_800171CC)(HSD_Archive** dst, const char* filename,
                         void* symbols, ...)
{
    HSD_Archive* archive;
    bool preloaded;
    va_list args;

    va_start(args, symbols);

    archive = lbDvd_8001819C(filename);
    if (archive != NULL) {
        preloaded = true;
    } else {
        archive = loadArchive(filename);
        preloaded = false;
    }

    vLoadSections(archive, symbols, args);

    va_end(args);

    if (dst != NULL) {
        *dst = archive;
    }
    return preloaded;
}

static inline void Locate(HSD_Archive* archive, intptr_t base_addr)
{
    u32 i;
    u32* ptr;

    for (i = 0; i < archive->header.nb_reloc; i++) {
        ptr = (u32*) (archive->data + archive->reloc_info[i].offset);
        *ptr += base_addr;
    }
}

int lbArchiveRelocate(HSD_Archive* archive, u8* src, size_t file_size,
                      intptr_t base_addr)
{
    size_t file_offset;

    if (archive == NULL) {
        return -1;
    }
#ifdef TARGET_PC
    /* The caller copies an already parsed native animation archive, then
     * adjusts its absolute pointers. Track the copy under its own allocation
     * so bounded animation readers can find it until the usual unregister. */
    HSD_ArchiveHeader header;
    if (!src || file_size < sizeof(header)) return -1;
    memcpy(&header, src, sizeof(header));
    if (header.file_size != file_size ||
        header.data_size > file_size - sizeof(header)) return -1;
    if (header.data_size &&
        !MeleePc_RegisterOwnedAssetRegion(src, src + sizeof(header),
                                         header.data_size)) return -1;
#endif
    memset(archive, 0, sizeof(HSD_Archive));
    archive->flags |= 1;
    memcpy(archive, src, sizeof(HSD_ArchiveHeader));

    if (archive->header.file_size != file_size) {
        OSReport("lbArchiveRelocate: byte-order mismatch! "
                 "Please check data format %x %x\n",
                 archive->header.file_size, file_size);
        return -1;
    }

    file_offset = sizeof(HSD_ArchiveHeader);
    if (archive->header.data_size != 0) {
        archive->data = src + file_offset;
        file_offset = archive->header.data_size + sizeof(HSD_ArchiveHeader);
    }
    if (archive->header.nb_reloc != 0) {
        archive->reloc_info = (HSD_ArchiveRelocationInfo*) (src + file_offset);
        file_offset +=
            archive->header.nb_reloc * sizeof(HSD_ArchiveRelocationInfo);
    }
    if (archive->header.nb_public != 0) {
        archive->public_info = (HSD_ArchivePublicInfo*) (src + file_offset);
        file_offset +=
            archive->header.nb_public * sizeof(HSD_ArchivePublicInfo);
    }
    if (archive->header.nb_extern != 0) {
        archive->extern_info = (HSD_ArchiveExternInfo*) (src + file_offset);
        file_offset +=
            archive->header.nb_extern * sizeof(HSD_ArchiveExternInfo);
    }
    if (file_offset < archive->header.file_size) {
        archive->symbols = (char*) (src + file_offset);
    }

    Locate(archive, base_addr);
#ifdef TARGET_PC
    if (getenv("MELEE_DIAGNOSTIC_ARCHIVE_COPY")) {
        static unsigned reports;
        if (reports++ < 64) {
            const void* base = NULL;
            const void* owner = NULL;
            uint32_t bytes = 0;
            int tracked = MeleePc_AssetRegionInfo(archive->data, &base, &owner,
                                                  &bytes);
            OSReport("NATIVE_ARCHIVE_COPY bytes=%u body=%u tracked=%d owner_matches=%d base_matches=%d size_matches=%d reloc=%u\n",
                     (u32) file_size, archive->header.data_size, tracked,
                     owner == src, base == archive->data,
                     bytes == archive->header.data_size, archive->header.nb_reloc);
        }
    }
#endif

    return 0;
}
