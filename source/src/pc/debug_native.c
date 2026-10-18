#include <sysdolphin/baselib/debug.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
/* Windows diagnostics do not inspect PowerPC registers or console FILE internals. */
void HSD_LogInit(void)
{
    fflush(stdout);
    fflush(stderr);
}
void HSD_Panic(const char* file, u32 line, const char* message)
{
    OSReport("HSD panic: %s:%u: %s\n", file ? file : "(unknown)", line,
             message ? message : "(no message)");
    _Exit(EXIT_FAILURE);
}
void __assert(const char* file, u32 line, const char* condition)
{
    OSReport("HSD assertion failed: %s\n", condition ? condition : "(unknown)");
    HSD_Panic(file, line, "assertion failure");
}

/* SDK panics must never return into partially failed game state. Avoid
 * heap allocation, GUI crash dialogs and global teardown from an arbitrary
 * failing callback thread. No card/save or original asset is written. */
void OSPanic(const char* file, int line, const char* format, ...)
{
    va_list arguments;
    fprintf(stderr, "OS panic: %s:%d: ", file ? file : "(unknown)", line);
    va_start(arguments, format);
    if (format != NULL) { vfprintf(stderr, format, arguments); }
    else { fputs("(no message)", stderr); }
    va_end(arguments);
    fputc('\n', stderr);
    fflush(stdout);
    fflush(stderr);
    _Exit(70);
}
