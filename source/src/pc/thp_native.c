#include <dolphin/thp/thp.h>
#include "movie_decode.h"
#include <string.h>
#define NATIVE_THP_READY 0x54485059U

void THPInit(void)
{
    /* Portable codec has no fixed-address locked cache or global work area. */
}

s32 THPDec_8032F8D4(u8* bytes, THPDec_8032FD40_Data* desc)
{
    unsigned width,height;
    if(!desc || !desc->encoded_length ||
       !MeleePc_MovieFrameInfo(bytes,desc->encoded_length,&width,&height)) return 0;
    desc->val0=0; desc->val1=width; desc->_pad=height; desc->val2=4;
    return 1;
}

s32 THPDec_8032FD40(THPDec_8032FD40_Data* desc, u16 height)
{
    if(!desc || desc->val2!=4 || !desc->val1 || desc->val1>640 ||
       (desc->val1&15) || !height || height>480 || (height&15)) return 0;
    return sizeof(THPFileInfo)+(size_t)desc->val1*height*3/2;
}

THPFileInfo* THPVideoDecode(void* header, void* status_out, THPFileInfo* work,
                            void* bytes, THPDec_8032FD40_Data* desc)
{
    const u16* dimensions=header;
    s32* status=status_out;
    size_t pixels;
    u8* planes;
    if(!status) return NULL;
    *status=1;
    if(!header || !work || !desc || !desc->encoded_length ||
       !THPDec_8032FD40(desc,dimensions[1]) || dimensions[0]!=desc->val1 ||
       dimensions[1]!=desc->_pad) return NULL;
    work->cnt=0;
    pixels=(size_t)dimensions[0]*dimensions[1];
    planes=(u8*)(work+1);
    if(!MeleePc_DecodeMovieFrame(bytes,desc->encoded_length,
                                 dimensions[0],dimensions[1],
                                 planes,pixels,planes+pixels,pixels/4,
                                 planes+pixels+pixels/4,pixels/4)) return NULL;
    memset(work,0,sizeof(*work));
    work->scratch=planes;
    work->xSize=dimensions[0]; work->ySize=dimensions[1];
    work->cnt=NATIVE_THP_READY;
    *status=0;
    return work;
}

static void copy_planes(THPFileInfo* work, void* y, void* u, void* v, u32 width)
{
    size_t pixels;
    if(!work || work->cnt!=NATIVE_THP_READY || width!=work->xSize ||
       !y || !u || !v) OSPanic(__FILE__,__LINE__,"Invalid native THP output state");
    pixels=(size_t)work->xSize*work->ySize;
    memmove(y,work->scratch,pixels);
    memmove(u,work->scratch+pixels,pixels/4);
    memmove(v,work->scratch+pixels+pixels/4,pixels/4);
}

void THPDec_80331340(THPFileInfo* work, void* y, void* u, void* v)
{ copy_planes(work,y,u,v,640); }
void THPDec_803313D0(THPFileInfo* work, void* y, void* u, void* v,u32 width)
{ copy_planes(work,y,u,v,width); }
