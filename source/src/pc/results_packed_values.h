#ifndef MELEE_PC_RESULTS_PACKED_VALUES_H
#define MELEE_PC_RESULTS_PACKED_VALUES_H
#include <stdint.h>
#include <string.h>
/* Original numeric words encode first halfword in the high bits. */
static void MeleePc_ResultsHalfwords(uint16_t out[4],uint32_t first,uint32_t second){
 out[0]=(uint16_t)(first>>16);out[1]=(uint16_t)first;
 out[2]=(uint16_t)(second>>16);out[3]=(uint16_t)second;
}
static void MeleePc_ResultsSignedHalfwords(int16_t out[4],uint32_t first,uint32_t second){
 uint16_t values[4];MeleePc_ResultsHalfwords(values,first,second);
 memcpy(out,values,sizeof(values));
}
#endif
