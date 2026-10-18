#include "movie_stream.h"
#include <string.h>

uint32_t MeleePc_MovieNextSize(const void* bytes)
{
    const unsigned char* p = bytes;
    return ((uint32_t)p[0]<<24) | ((uint32_t)p[1]<<16) |
           ((uint32_t)p[2]<<8) | p[3];
}

int MeleePc_MovieReadFits(uint32_t offset, uint32_t size,
                          uint32_t capacity, uint32_t file_size)
{
    return size >= 4 && size <= capacity && offset <= file_size &&
           size <= file_size - offset;
}

int MeleePc_MovieHeader(const void* bytes, size_t length, uint32_t file_size,
                        uint32_t values[9], uint32_t* capacity)
{
    const unsigned char* p = bytes;
    uint32_t v[9], c;
    unsigned i;
    if (!p || length < 64 || !values || !capacity || memcmp(p,"MTHP",4))
        return 0;
    for (i=0;i<9;i++) v[i]=MeleePc_MovieNextSize(p+8+i*4);
    /* Native baseline decoder supports tiled 4:2:0; reject huge allocations. */
    if (v[0]!=2 || v[1]<4 || v[1]>16*1024*1024 ||
        !v[2] || v[2]>640 || (v[2]&15) ||
        !v[3] || v[3]>480 || (v[3]&15) ||
        !v[4] || v[4]>60 || !v[5] || v[5]>1000000 ||
        v[6]<64 || v[7]) return 0;
    /* Retail streams include one alignment block beyond their declared max. */
    c=(v[1]+63)&~(uint32_t)31;
    if (!MeleePc_MovieReadFits(v[6],v[8],c,file_size)) return 0;
    memcpy(values,v,sizeof(v));
    *capacity=c;
    return 1;
}
