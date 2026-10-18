#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
enum { AURORA_MESH_PRIMITIVES=16, AURORA_MESH_POINTS=8 };
typedef struct AuroraMeshPoint { double position[3],directions[9],uv[8][2]; uint32_t selector; uint8_t color[2][4]; } AuroraMeshPoint;
typedef struct AuroraMeshUv { uint32_t storage,offset,components,type,fraction,stride; } AuroraMeshUv;
typedef struct AuroraMeshTexgen { uint32_t source,postMatrix,sourceSrtBytes;double matrix[12]; } AuroraMeshTexgen;
typedef struct AuroraMeshChannel {
    uint32_t enabled,ambientSource,materialSource,lightMask,diffuse,attenuation;
    uint8_t ambient[4],material[4];
} AuroraMeshChannel;
typedef struct AuroraMeshLight {double position[3],direction[3],color[4],cosine[3],distance[3];} AuroraMeshLight;
typedef struct AuroraMeshPrimitive {
    uint32_t offset,bytes,vertices,points;
    AuroraMeshPoint point[AURORA_MESH_POINTS];
} AuroraMeshPrimitive;
typedef struct AuroraOriginalMesh {
    const void* display;
    const void* original;
    uint32_t bytes,frame,actor,nbt,primitives;
    uint32_t stride,positionOffset,positionStorage,positionComponents,positionType,positionFraction,positionStride;
    uint32_t paletteMask,normalMask,paletteKind,sourceBoneChecks,sourceWeightChecks,sourceBindComponents,weightedSlots,rightSlots;
    double positionPalette[10][12],normalPalette[10][9];
    uint32_t normalComponents,normalOffset,normalStorage,normalType,normalStride,normalNbt3;
    uint32_t lightingReference,sourceMaterialChecks,lightMask,channelMask;
    uint32_t colorStorage[2],colorOffset[2],colorType[2],colorStride[2];
    uint32_t uvMask;AuroraMeshUv uv[8];
    uint32_t texgenMask;AuroraMeshTexgen texgen[8];
    AuroraMeshChannel channels[4];AuroraMeshLight lights[8];
    AuroraMeshPrimitive primitive[AURORA_MESH_PRIMITIVES];
} AuroraOriginalMesh;
/* Explicit GAME_STARTUP+ORIGINAL_MESH_GPU diagnostic only. Deep-copies original
 * bytes/points in RAM for the immediately following matching GXCallDisplayList.
 * Display address is retained only as identity;byte/point data are copied.
 * No original payload is exported.
 * Return1 means registered,not submitted/rendered/GPU-verified. */
int aurora_diagnostic_original_mesh(const AuroraOriginalMesh* mesh);
#ifdef __cplusplus
}
#endif
