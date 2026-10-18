#include "lbfile.h"

#include <placeholder.h>
#include <string.h>
#ifdef TARGET_PC
#include <stdlib.h>
#endif

#include "lb_0195.h"
#include "lbdvd.h"
#include "lbheap.h"
#include "lblanguage.h"
#include <dolphin/dvd.h>
#include <sysdolphin/baselib/debug.h>
#include <sysdolphin/baselib/devcom.h>
#ifdef TARGET_PC
#include <pc/arena_range.h>
#include <dolphin/ar.h>
#include <dolphin/os.h>
#include <windows.h>
#endif

static bool cancel;

static void lbFile_8001615C(int dcreq, uintptr_t args, void* buf,
                            bool cancelflag)
{
    HSD_ASSERT(71, !cancelflag);
    cancel = true;
}

/// @todo Non-inlined function forces loop in ::lbFile_800161C4 to yield to
///       interrupts. Pragma solution likely fake.
#ifdef __MWERKS__
#pragma push
#pragma dont_inline on
#endif
static bool discIsDone(void)
{
    lb_800195D0();
    return cancel;
}
#ifdef __MWERKS__
#pragma pop
#endif

static void waitForDisc(void)
{
    do {
    } while (!discIsDone());
}

void lbFile_800161C4(int file, uintptr_t src, uintptr_t dst, size_t size,
                     int type, int pri)
{
    cancel = false;
    HSD_DevComRequest(file, src, dst, size, type, pri, lbFile_8001615C, 0);
    waitForDisc();
}

const int FILE_EXTENSION_LENGTH = 4; // ".usd" or ".dat"
#ifdef TARGET_PC
/* Trophy/intro resources include archive symbols longer than the original
 * console buffer (for example ToyDspFgseriesA_Top_matanim_joint). */
#define MAX_FILENAME_LENGTH 0x100
#define MAX_BASENAME_LENGTH (MAX_FILENAME_LENGTH - FILE_EXTENSION_LENGTH - 1)
#else
#define MAX_FILENAME_LENGTH 0x20
#define MAX_BASENAME_LENGTH (MAX_FILENAME_LENGTH - FILE_EXTENSION_LENGTH)
#endif

/// append file extension (if needed)
char* lbFileGetFullName(const char* basename)
{
    static char result[MAX_FILENAME_LENGTH];
    const char* cur = basename;
    int pos = 0;

    while (*cur != '\0' && *cur != '.') {
        // no room for file extension?
#ifdef TARGET_PC
        if (pos >= MAX_BASENAME_LENGTH) {
#else
        if (pos > MAX_BASENAME_LENGTH) {
#endif
            OSReport("Error : file name too long %s.", basename);
            HSD_ASSERT(0x99, NULL);
            return result;
        }
        result[pos++] = *cur++;
    }
    // keep any existing file extension
    if (cur[0] != '\0' && cur[1] != '\0') {
#ifdef TARGET_PC
        if (strlen(basename) >= MAX_FILENAME_LENGTH) {
            OSReport("Error : file name too long %s.", basename);
            HSD_ASSERT(0x99, NULL);
            return result;
        }
#endif
        strcpy(result, basename);
        // otherwise, append the appropriate extension for the locale
    } else if (*cur == '.') {
        result[pos++] = '.';
        if (lbLang_IsSettingUS()) {
            strcpy(&result[pos], "usd");
        } else {
            strcpy(&result[pos], "dat");
        }
    } else {
        result[pos++] = '.';
        if (lbLang_IsSavedLanguageUS()) {
            strcpy(&result[pos], "usd");
        } else {
            strcpy(&result[pos], "dat");
        }
    }
    return result;
}

size_t lbFile_8001634C(int fileno)
{
    DVDFileInfo info;
    size_t length;
    bool intr = OSDisableInterrupts();

    if (!DVDFastOpen(fileno, &info)) {
        OSReport("Cannot open file no=%d.", fileno);
        HSD_ASSERT(0xD8, 0);
    }

    length = info.length;
    DVDClose(&info);
    OSRestoreInterrupts(intr);
    return length;
}

size_t lbFileGetSize(const char* basename)
{
    int entry_num;
    char* filename = lbFileGetFullName(basename);
    entry_num = DVDConvertPathToEntrynum(filename);
#ifdef TARGET_PC
    if (entry_num == -1 && getenv("MELEE_DIAGNOSTIC_GAME_STARTUP")) {
        void* caller = __builtin_return_address(0);
        void* image_base = GetModuleHandleA(NULL);
        OSReport("NATIVE_DVD_LOOKUP_CALLER path=%s return_address=%p image_base=%p image_rva=%p\n",
                 filename, caller, image_base,
                 (void*) ((uintptr_t) caller - (uintptr_t) image_base));
        char current_dir[256] = { 0 };
        char root_path[MAX_FILENAME_LENGTH + 2];
        char alternate_path[MAX_FILENAME_LENGTH + 2];
        size_t filename_length = strlen(filename);
        DVDGetCurrentDir(current_dir, sizeof(current_dir));
        root_path[0] = '/';
        memcpy(root_path + 1, filename, filename_length + 1);
        memcpy(alternate_path, root_path, filename_length + 2);
        if (filename_length >= 4 &&
            strcmp(filename + filename_length - 4, ".usd") == 0) {
            memcpy(alternate_path + filename_length - 3, ".dat", 5);
        }
        OSReport("NATIVE_DVD_MISSING path=%s current_dir=%s root_lookup=%d dat_lookup=%d\n",
                 filename, current_dir, DVDConvertPathToEntrynum(root_path),
                 DVDConvertPathToEntrynum(alternate_path));
        {
            DVDDir root;
            DVDDirEntry entry;
            unsigned directories = 0;
            if (DVDOpenDir("/", &root)) {
                while (DVDReadDir(&root, &entry)) {
                    if (entry.isDir) {
                        OSReport("NATIVE_DVD_ROOT_DIR name=%s entry=%u\n",
                                 entry.name != NULL ? entry.name : "<unnamed>",
                                 entry.entryNum);
                        ++directories;
                    }
                }
                DVDCloseDir(&root);
            }
            OSReport("NATIVE_DVD_ROOT_DIR_COUNT logged=%u\n", directories);
        }
    }
#endif
    HSD_ASSERTREPORT(0xEE, entry_num != -1, "file isn't exist %s = %d\n",
                     filename, entry_num);
    return lbFile_8001634C(entry_num);
}

void lbFile_800164A4(int file, uintptr_t dst, size_t* size, int pri,
                     HSD_DevComCallback callback, uintptr_t args)
{
    int type;
    *size = lbFile_8001634C(file);
#ifdef TARGET_PC
    uintptr_t cpu_lo=(uintptr_t)OSPhysicalToCached(0);
    uintptr_t cpu_size=OSGetPhysicalMemSize();
    size_t transfer_size=OSRoundUp32B(*size);
    HSD_ASSERT(1,transfer_size && dst<=UINTPTR_MAX-transfer_size &&
                 cpu_lo<=UINTPTR_MAX-cpu_size);
    int destination=MeleePc_ArenaRange(dst,dst+transfer_size,
                    cpu_lo,cpu_lo+cpu_size,0,ARGetSize());
    HSD_ASSERTREPORT(1,destination,"Native DVD destination outside MEM1/ARAM.\n");
    type=destination==2 ? 0x21 : 0x23;
#else
    type = (dst >= 0x80000000) ? 0x21 : 0x23;
#endif
    HSD_DevComRequest(file, 0, dst, OSRoundUp32B(*size), type, pri, callback,
                      args);
}

void lbFile_80016580(const char* basename, void* dst, size_t* size,
                     HSD_DevComCallback callback, uintptr_t args)
{
    char* filename = lbFileGetFullName(basename);
    int entry_num = DVDConvertPathToEntrynum(filename);
    PAD_STACK(4);

    HSD_ASSERTREPORT(0x11A, entry_num != -1, "file isn't exist %s = %d\n",
                     filename, entry_num);

    lbFile_800164A4(entry_num, (uintptr_t) dst, size, 1, callback, args);
}

void lbFile_8001668C(const char* basename, void* dst, size_t* size)
{
    cancel = false;
    lbFile_80016580(basename, dst, size, lbFile_8001615C, 0);
    waitForDisc();
}

static void loadFile(int heap_id, const char* basename, void** dst,
                     size_t* size)
{
    *size = lbFileGetSize(basename);
    *dst = lbHeap_80015BD0(heap_id, OSRoundUp32B(*size));
    lbFile_80016580(basename, *dst, size, lbFile_8001615C, 0);
    waitForDisc();
}

void lbFile_80016760(const char* basename, void** dst, size_t* size)
{
    cancel = false;
    loadFile(0, basename, dst, size);
}

bool lbFile_800168A0(int heap_id, const char* basename, void** dst,
                     size_t* size)
{
    if ((*dst = lbDvd_8001819C(basename))) {
        *size = lbFileGetSize(basename);
        return true;
    } else {
        cancel = false;
        loadFile(heap_id, basename, dst, size);
        return false;
    }
}
