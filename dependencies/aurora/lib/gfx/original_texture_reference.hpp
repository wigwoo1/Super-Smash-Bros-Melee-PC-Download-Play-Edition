#pragma once
#include <cstdint>
#include <cstddef>
#include <array>
#include <vector>
namespace aurora::gfx::original_texture {
/* Independent per-texel CMPR address/selector arithmetic. No calls to production
 * texture conversion or upload buffers. Original bytes stay in owned RAM. */
inline bool cmpr_pixel(const uint8_t* p,size_t bytes,unsigned w,unsigned h,unsigned x,unsigned y,std::array<uint8_t,4>& out){
  if(!p||!w||!h||w>512||h>512||x>=w||y>=h||bytes!=size_t((w+7)/8)*((h+7)/8)*32)return false;
  const size_t block=(size_t(y/8)*((w+7)/8)+x/8)*32+((y%8)/4*2+(x%8)/4)*8;
  const unsigned a=p[block]*256u+p[block+1],b=p[block+2]*256u+p[block+3];
  const unsigned selector=(p[block+4+y%4]>>(6-2*(x%4)))&3;
  std::array<unsigned,3> ca{a>>11,(a>>5)&63,a&31},cb{b>>11,(b>>5)&63,b&31};
  for(unsigned c=0;c<3;c++){
    const unsigned bits=c==1?6:5;const unsigned first=(ca[c]<<(8-bits))|(ca[c]>>(2*bits-8));
    const unsigned second=(cb[c]<<(8-bits))|(cb[c]>>(2*bits-8));
    out[c]=selector==0?first:selector==1?second:a>b?
      (selector==2?(first*5+second*3)/8:(first*3+second*5)/8):(first+second)/2;
  }
  out[3]=a<=b&&selector==3?0:255;return true;
}
inline bool decode_cmpr(const std::vector<uint8_t>& bytes,unsigned w,unsigned h,std::vector<uint8_t>& rgba){
  if(!w||!h||w>512||h>512||bytes.size()!=size_t((w+7)/8)*((h+7)/8)*32)return false;
  std::vector<uint8_t> result(size_t(w)*h*4);std::array<uint8_t,4> p{};
  for(unsigned y=0;y<h;y++)for(unsigned x=0;x<w;x++){
    if(!cmpr_pixel(bytes.data(),bytes.size(),w,h,x,y,p))return false;
    for(unsigned c=0;c<4;c++)result[(size_t(y)*w+x)*4+c]=p[c];
  }
  rgba=std::move(result);return true;
}
/* C8 tile addressing and original BE TLUT expansion,independent of production
 * conversion. Includes padded NPOT tiles;out-of-range palette indices refuse
 * atomically instead of inventing a colour. */
inline bool decode_original(const std::vector<uint8_t>& bytes,const std::vector<uint8_t>& palette,unsigned format,unsigned paletteFormat,unsigned w,unsigned h,std::vector<uint8_t>& rgba){
  if(format==14)return palette.empty()&&decode_cmpr(bytes,w,h,rgba);
  if(format!=9||!w||!h||w>512||h>512||bytes.size()!=size_t((w+7)/8)*((h+3)/4)*32||palette.empty()||palette.size()>512||palette.size()%2||paletteFormat>2)return false;
  std::vector<uint8_t> result(size_t(w)*h*4);
  for(unsigned y=0;y<h;y++)for(unsigned x=0;x<w;x++){
    const unsigned index=bytes[(size_t(y/4)*((w+7)/8)+x/8)*32+(y%4)*8+x%8];if(index*2+1>=palette.size())return false;
    const unsigned word=unsigned(palette[index*2])*256+palette[index*2+1];const size_t at=(size_t(y)*w+x)*4;
    if(paletteFormat==0){result[at]=result[at+1]=result[at+2]=word&255;result[at+3]=word>>8;}
    else if(paletteFormat==1){const unsigned r=word>>11,g=(word>>5)&63,b=word&31;result[at]=(r<<3)|(r>>2);result[at+1]=(g<<2)|(g>>4);result[at+2]=(b<<3)|(b>>2);result[at+3]=255;}
    else if(word&0x8000){const unsigned r=(word>>10)&31,g=(word>>5)&31,b=word&31;result[at]=(r<<3)|(r>>2);result[at+1]=(g<<3)|(g>>2);result[at+2]=(b<<3)|(b>>2);result[at+3]=255;}
    else{const unsigned a=(word>>12)&7;result[at]=((word>>8)&15)*17;result[at+1]=((word>>4)&15)*17;result[at+2]=(word&15)*17;result[at+3]=(a<<5)|(a<<2)|(a>>1);}
  }
  rgba=std::move(result);return true;
}
}
