#ifndef MELEE_PC_RENDER_PROJECTION_H
#define MELEE_PC_RENDER_PROJECTION_H
#include <math.h>
/* Pure render-only matrix transform. Never mutate the authored camera.
 * Caller must restrict this to registered gameplay or HUD camera projections.
 * Orthographic HUD use centers a logical-aspect safe area, including X offsets.
 * Actual texture targets and split viewport paths require their own policy. */
static inline int MeleePc_WidenPerspective(const float source[4][4],
                                         float output[4][4],
                                         float logical_aspect,
                                         float display_aspect)
{
    unsigned row, col;
    float scale;
    if (!source || !output || source == output ||
        !isfinite(logical_aspect) || !isfinite(display_aspect) ||
        logical_aspect <= 0 || display_aspect < logical_aspect) return 0;
    scale = logical_aspect / display_aspect;
    for (row=0; row<4; ++row) for (col=0; col<4; ++col)
        if (!isfinite(source[row][col])) return 0;
    for (row=0; row<4; ++row) for (col=0; col<4; ++col)
        output[row][col] = source[row][col] * (row == 0 ? scale : 1.0f);
    return 1;
}
/* Screen-copy texture coordinates are homogeneous S/T/Q. Match widened
 * scene X around U=0.5: S' = scale*S + (1-scale)*0.5*Q.
 * Caller must use the same accepted main-camera display ratio as rendering. */
static inline int MeleePc_WidenScreenTexture(const float source[3][4],
                                            float output[3][4],
                                            float logical_aspect,
                                            float display_aspect)
{
    unsigned row,col;
    float scale;
    if (!source || !output || source == output ||
        !isfinite(logical_aspect) || !isfinite(display_aspect) ||
        logical_aspect <= 0 || display_aspect < logical_aspect) return 0;
    scale=logical_aspect/display_aspect;
    for(row=0;row<3;++row)for(col=0;col<4;++col)
        if(!isfinite(source[row][col]))return 0;
    for(row=0;row<3;++row)for(col=0;col<4;++col)
        output[row][col]=row==0 ? scale*source[0][col]+(1-scale)*0.5f*source[2][col] : source[row][col];
    return 1;
}
#endif
