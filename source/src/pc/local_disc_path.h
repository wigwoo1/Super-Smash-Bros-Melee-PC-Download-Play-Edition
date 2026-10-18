#ifndef MELEE_PC_LOCAL_DISC_PATH_H
#define MELEE_PC_LOCAL_DISC_PATH_H

#include <stddef.h>
#include <string.h>

/* Resolve the private, user-supplied game image beside the installed host.
 * Keeping this pure lets the path policy be checked without opening an image. */
static inline int MeleePc_ResolveBundledDiscPath(const char* executable_path,
                                                  char* output,
                                                  size_t output_size)
{
    static const char data_directory[] = "data";
    static const char image_name[] = "melee.nkit.iso";
    const char* slash;
    const char* backslash;
    size_t prefix_size;
    size_t required_size;

    if (executable_path == NULL || output == NULL || output_size == 0 ||
        executable_path[0] == '\0') {
        return 0;
    }

    slash = strrchr(executable_path, '/');
    backslash = strrchr(executable_path, '\\');
    if (slash == NULL) {
        slash = backslash;
    } else if (backslash != NULL && backslash > slash) {
        slash = backslash;
    }
    if (slash == NULL) {
        return 0;
    }

    prefix_size = (size_t) (slash - executable_path) + 1;
    required_size = prefix_size + sizeof(data_directory) + sizeof(image_name);
    if (required_size > output_size) {
        return 0;
    }

    memcpy(output, executable_path, prefix_size);
    memcpy(output + prefix_size, data_directory, sizeof(data_directory) - 1);
    output[prefix_size + sizeof(data_directory) - 1] = *slash;
    memcpy(output + prefix_size + sizeof(data_directory), image_name,
           sizeof(image_name));
    return 1;
}

#endif
