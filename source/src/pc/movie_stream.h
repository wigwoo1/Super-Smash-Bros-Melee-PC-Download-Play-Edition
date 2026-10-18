#ifndef MELEE_PC_MOVIE_STREAM_H
#define MELEE_PC_MOVIE_STREAM_H
#include <stddef.h>
#include <stdint.h>
/* Values at offsets 8..40, returned in host order only after validation. */
int MeleePc_MovieHeader(const void* bytes, size_t length, uint32_t file_size,
                        uint32_t values[9], uint32_t* capacity);
uint32_t MeleePc_MovieNextSize(const void* bytes);
int MeleePc_MovieReadFits(uint32_t offset, uint32_t size,
                          uint32_t capacity, uint32_t file_size);
#endif
