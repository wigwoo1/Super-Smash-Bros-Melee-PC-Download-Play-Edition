#include <stdarg.h>
#include <stdio.h>
/* Native diagnostics are real output, not success-only runtime stubs. */
void OSVReport(const char* message, va_list arguments)
{
    if (message != NULL) { vfprintf(stderr, message, arguments); fflush(stderr); }
}
void OSReport(const char* message, ...)
{
    va_list arguments;
    va_start(arguments, message);
    OSVReport(message, arguments);
    va_end(arguments);
}
