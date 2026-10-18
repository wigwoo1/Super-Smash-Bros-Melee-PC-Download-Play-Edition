#ifndef MELEE_PC_INDEXED_VERTEX_EXTENT_H
#define MELEE_PC_INDEXED_VERTEX_EXTENT_H
#include <stdint.h>
#include <stddef.h>
/* Packed GX indices stay big endian. Caller supplies the source-derived
 * attribute offset/layout and actual scalar read width, including NBT3.
 * Failure leaves the output unchanged. No memory is read from the array. */
static int MeleePc_IndexedVertexSliceExtent(const unsigned char* vertices,
 size_t bytes,uint32_t count,uint32_t vertex_stride,uint32_t index_offset,
 uint32_t index_width,uint32_t indices_per_vertex,uint32_t array_stride,
 uint32_t element_bytes,uint32_t slice_offset_step,uint32_t array_bound,uint32_t* extent){
 if(!extent || (count&&!vertices) || !vertex_stride || !array_stride ||
    !element_bytes || (index_width!=1&&index_width!=2) ||
    !indices_per_vertex || indices_per_vertex>3)return 0;
 if(index_offset>vertex_stride ||
    indices_per_vertex>(vertex_stride-index_offset)/index_width)return 0;
 if(count>bytes/vertex_stride)return 0;
 uint32_t needed=0;
 for(uint32_t i=0;i<count;i++){
  const unsigned char* v=vertices+(size_t)i*vertex_stride+index_offset;
  for(uint32_t j=0;j<indices_per_vertex;j++){
   uint32_t index=v[j*index_width];
   if(index_width==2)index=(index<<8)|v[j*index_width+1];
   uint64_t end=(uint64_t)index*array_stride+(uint64_t)j*slice_offset_step+element_bytes;
   if(end>array_bound || end>UINT32_MAX)return 0;
   if(end>needed)needed=(uint32_t)end;
  }
 }
 *extent=needed;return 1;
}
static int MeleePc_IndexedVertexExtent(const unsigned char* vertices,
 size_t bytes,uint32_t count,uint32_t vertex_stride,uint32_t index_offset,
 uint32_t index_width,uint32_t indices_per_vertex,uint32_t array_stride,
 uint32_t element_bytes,uint32_t array_bound,uint32_t* extent){
 return MeleePc_IndexedVertexSliceExtent(vertices,bytes,count,vertex_stride,
  index_offset,index_width,indices_per_vertex,array_stride,element_bytes,0,array_bound,extent);
}
#endif
