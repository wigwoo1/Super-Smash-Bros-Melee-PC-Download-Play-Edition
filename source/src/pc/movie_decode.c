#include "movie_decode.h"
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_JPEG
#define STBI_NO_STDIO
#define STBI_NO_SIMD
#define STBI_MAX_DIMENSIONS 640
#include "third_party/stb_image.h"

/* THP's entropy bytes are not JPEG byte-stuffed. Preserve all header segments
 * exactly, then stuff entropy FF bytes for the bounded portable JPEG decoder.
 * This implementation supports baseline interleaved 4:2:0 without restarts. */
static size_t scan_offset(const unsigned char* p, size_t length,
                           unsigned width, unsigned height,
                           unsigned* found_width, unsigned* found_height)
{
    size_t pos=2;
    int frame=0;
    if(length<2 || p[0]!=255 || p[1]!=216) return 0;
    while(pos<length) {
        unsigned marker, n;
        if(p[pos++]!=255) return 0;
        while(pos<length && p[pos]==255) ++pos;
        if(pos>=length) return 0;
        marker=p[pos++];
        if(length-pos<2) return 0;
        n=((unsigned)p[pos]<<8)|p[pos+1];
        if(n<2 || n>length-pos) return 0;
        if(marker==0xc0) {
            unsigned w,h;
            if(n!=17) return 0;
            h=((unsigned)p[pos+3]<<8)|p[pos+4];
            w=((unsigned)p[pos+5]<<8)|p[pos+6];
            if(n!=17 || p[pos+2]!=8 || p[pos+7]!=3 ||
               !w || w>640 || (w&15) || !h || h>480 || (h&15) ||
               (height && h!=height) || (width && w!=width) ||
               p[pos+9]!=0x22 || p[pos+12]!=0x11 || p[pos+15]!=0x11)
                return 0;
            if(found_width) *found_width=w;
            if(found_height) *found_height=h;
            frame=1;
        } else if(marker==0xc2 || marker==0xdd) return 0;
        else if(marker==0xda) {
            if(!frame || n!=12 || p[pos+2]!=3 ||
               p[pos+9]!=0 || p[pos+10]!=63 || p[pos+11]!=0) return 0;
            return pos+n;
        }
        pos+=n;
    }
    return 0;
}

int MeleePc_MovieFrameInfo(const void* bytes, size_t length,
                           unsigned* width, unsigned* height)
{
    unsigned w=0,h=0;
    if(!bytes || !width || !height || length>16*1024*1024 ||
       !scan_offset(bytes,length,0,0,&w,&h)) return 0;
    *width=w; *height=h;
    return 1;
}

static void tile(unsigned char* out, const unsigned char* in,
                  unsigned width, unsigned height, unsigned stride)
{
    unsigned by,bx,y,x;
    for(by=0;by<height;by+=4) for(bx=0;bx<width;bx+=8)
        for(y=0;y<4;y++) for(x=0;x<8;x++)
            *out++=in[(by+y)*stride+bx+x];
}

int MeleePc_DecodeMovieFrame(const void* bytes, size_t length,
                             unsigned width, unsigned height,
                             void* y, size_t y_size, void* u, size_t u_size,
                             void* v, size_t v_size)
{
    const unsigned char* p=bytes;
    unsigned char* jpeg;
    stbi__context context;
    stbi__jpeg decoder;
    size_t scan,i,used,pixels;
    int ok=0;
    if(!p || !y || !u || !v || !width || width>640 || (width&15) ||
       !height || height>480 || (height&15) || length>16*1024*1024) return 0;
    pixels=(size_t)width*height;
    if(y_size<pixels || u_size<pixels/4 || v_size<pixels/4) return 0;
    scan=scan_offset(p,length,width,height,NULL,NULL);
    if(!scan || scan==length) return 0;
    jpeg=malloc(length*2+2);
    if(!jpeg) return 0;
    memcpy(jpeg,p,scan); used=scan;
    for(i=scan;i<length;i++) {
        jpeg[used++]=p[i];
        if(p[i]==255) jpeg[used++]=0;
    }
    jpeg[used++]=255; jpeg[used++]=217;
    memset(&context,0,sizeof(context));
    memset(&decoder,0,sizeof(decoder));
    stbi__start_mem(&context,jpeg,(int)used);
    decoder.s=&context;
    stbi__setup_jpeg(&decoder);
    if(stbi__decode_jpeg_image(&decoder) && context.img_n==3 &&
       context.img_x==width && context.img_y==height && !decoder.progressive &&
       decoder.img_comp[0].h==2 && decoder.img_comp[0].v==2 &&
       decoder.img_comp[1].h==1 && decoder.img_comp[1].v==1 &&
       decoder.img_comp[2].h==1 && decoder.img_comp[2].v==1) {
        tile(y,decoder.img_comp[0].data,width,height,decoder.img_comp[0].w2);
        tile(u,decoder.img_comp[1].data,width/2,height/2,decoder.img_comp[1].w2);
        tile(v,decoder.img_comp[2].data,width/2,height/2,decoder.img_comp[2].w2);
        ok=1;
    }
    stbi__cleanup_jpeg(&decoder);
    free(jpeg);
    return ok;
}
