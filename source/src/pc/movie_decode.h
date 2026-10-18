#ifndef MELEE_PC_MOVIE_DECODE_H
#define MELEE_PC_MOVIE_DECODE_H
#include <stddef.h>
int MeleePc_MovieFrameInfo(const void* bytes, size_t length,
                           unsigned* width, unsigned* height);
/* Decode bounded THP raw-entropy JPEG into GX I8 8x4 tiled Y/U/V planes.
 * Output capacities must be width*height and width*height/4 respectively.
 * No output is modified on failure. No console IDCT bit-identity claim. */
int MeleePc_DecodeMovieFrame(const void* bytes, size_t length,
                             unsigned width, unsigned height,
                             void* y, size_t y_size, void* u, size_t u_size,
                             void* v, size_t v_size);
#endif
