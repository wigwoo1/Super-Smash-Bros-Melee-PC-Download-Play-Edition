/* Optional bounded live Euler/quaternion transform oracle. Read-only. */
#ifndef MELEE_JOBJ_MATRIX_DIAGNOSTIC_H
#define MELEE_JOBJ_MATRIX_DIAGNOSTIC_H
#ifdef TARGET_PC
#include <stdlib.h>
#include <dolphin/os.h>
unsigned int HSD_JObjDiagnosticMatchFrame;
static int JObjDiagnosticCompareMatrix(const HSD_JObj* j, double* max_error)
{
    double rx[3][3], ry[3][3], rz[3][3], tmp[3][3], rot[3][3];
    double local[3][4], expected[3][4], ps[3], scale[3], trans[3];
    double x, y, z, error, maximum = 0;
    int a, b, k, failures = 0;
    x = j->rotate.x; y = j->rotate.y; z = j->rotate.z;
    if (j->flags & JOBJ_USE_QUATERNION) {
        /* Independent double axis-angle/Rodrigues reference; C_MTXQuat uses
         * a float quadratic expansion and divides by the quaternion norm. */
        double w=j->rotate.w, length=sqrt(x*x+y*y+z*z);
        double norm=sqrt(x*x+y*y+z*z+w*w), angle, c, sine, axis[3]={1,0,0};
        if (!(norm>0) || !isfinite(norm)) { *max_error=INFINITY; return 12; }
        angle=2*atan2(length,w); c=cos(angle); sine=sin(angle);
        if(length>0) { axis[0]=x/length; axis[1]=y/length; axis[2]=z/length; }
        for(a=0;a<3;a++) for(b=0;b<3;b++)
            rot[a][b]=(a==b?c:0)+(1-c)*axis[a]*axis[b];
        rot[0][1]-=sine*axis[2]; rot[0][2]+=sine*axis[1];
        rot[1][0]+=sine*axis[2]; rot[1][2]-=sine*axis[0];
        rot[2][0]-=sine*axis[1]; rot[2][1]+=sine*axis[0];
    } else {
        memset(rx, 0, sizeof(rx)); memset(ry, 0, sizeof(ry)); memset(rz, 0, sizeof(rz));
        rx[0][0]=1; rx[1][1]=cos(x); rx[1][2]=-sin(x); rx[2][1]=sin(x); rx[2][2]=cos(x);
        ry[0][0]=cos(y); ry[0][2]=sin(y); ry[1][1]=1; ry[2][0]=-sin(y); ry[2][2]=cos(y);
        rz[0][0]=cos(z); rz[0][1]=-sin(z); rz[1][0]=sin(z); rz[1][1]=cos(z); rz[2][2]=1;
        for(a=0;a<3;a++) for(b=0;b<3;b++) {
            tmp[a][b]=0; for(k=0;k<3;k++) tmp[a][b]+=ry[a][k]*rx[k][b];
        }
        for(a=0;a<3;a++) for(b=0;b<3;b++) {
            rot[a][b]=0; for(k=0;k<3;k++) rot[a][b]+=rz[a][k]*tmp[k][b];
        }
    }
    ps[0]=ps[1]=ps[2]=1;
    if(j->parent && j->parent->scl) {
        ps[0]=j->parent->scl->x; ps[1]=j->parent->scl->y; ps[2]=j->parent->scl->z;
    }
    scale[0]=j->scale.x; scale[1]=j->scale.y; scale[2]=j->scale.z;
    trans[0]=j->translate.x; trans[1]=j->translate.y; trans[2]=j->translate.z;
    for(a=0;a<3;a++) {
        for(b=0;b<3;b++) local[a][b]=rot[a][b]*scale[b]*ps[b]/ps[a];
        local[a][3]=trans[a];
    }
    for(a=0;a<3;a++) for(b=0;b<4;b++) {
        expected[a][b]=local[a][b];
        if(j->parent) {
            expected[a][b]=(b==3)?j->parent->mtx[a][3]:0;
            for(k=0;k<3;k++) expected[a][b]+=j->parent->mtx[a][k]*local[k][b];
        }
    }
    if(j->aobj && j->aobj->hsd_obj) {
        HSD_JObj* other=(HSD_JObj*)j->aobj->hsd_obj;
        for(a=0;a<3;a++) {
            expected[a][3]=other->mtx[a][3];
            for(k=0;k<3;k++) expected[a][3]+=other->mtx[a][k]*trans[k];
        }
    }
    for(a=0;a<3;a++) for(b=0;b<4;b++) {
        error=fabs(j->mtx[a][b]-expected[a][b]);
        if(!isfinite(j->mtx[a][b]) || !isfinite(expected[a][b]) ||
           !(error<=2e-5*(1+fabs(expected[a][b])))) failures++;
        if(error>maximum) maximum=error;
    }
    *max_error=maximum;
    return failures;
}
static void JObjDiagnosticMatrix(HSD_JObj* j)
{
    static unsigned int count, quaternion_count, quaternion_frame, per_frame;
    double maximum;
    int failures, quaternion=(j->flags & JOBJ_USE_QUATERNION)!=0;
    if (HSD_JObjDiagnosticMatchFrame < 300 ||
        !getenv("MELEE_DIAGNOSTIC_GAME_STARTUP") ||
        !getenv("MELEE_DIAGNOSTIC_JOINT_MATRIX")) return;
    if(quaternion) {
        /* Preserve the existing Euler contract. Quaternion samples have their
         * own opt-in and budget, spread over at least32 simulation frames. */
        if(!getenv("MELEE_DIAGNOSTIC_JOINT_QUATERNION") || quaternion_count>=256) return;
        if(quaternion_frame!=HSD_JObjDiagnosticMatchFrame) {
            quaternion_frame=HSD_JObjDiagnosticMatchFrame;per_frame=0;
        }
        if(per_frame>=8) return;
        ++per_frame;++quaternion_count;
    } else {
        if(count>=256) return;
        ++count;
    }
    failures=JObjDiagnosticCompareMatrix(j,&maximum);
    if(quaternion) {
        OSReport("NATIVE_JOINT_QUAT_MATRIX sample=%u frame=%u parent=%d aobj=%d mismatches=%d max_error=%.9g joint=%p\n",
            quaternion_count,HSD_JObjDiagnosticMatchFrame,j->parent!=NULL,
            j->aobj && j->aobj->hsd_obj,failures,maximum,(void*)j);
    } else {
        OSReport("NATIVE_JOINT_MATRIX sample=%u frame=%u parent=%d aobj=%d mismatches=%d max_error=%f\n",
            count,HSD_JObjDiagnosticMatchFrame,j->parent!=NULL,
            j->aobj && j->aobj->hsd_obj,failures,maximum);
    }
}
#endif
#endif
