/* Optional read-only blend oracle. Double Hamilton products and relative
 * axis-angle interpolation deliberately differ from the game's sine weights.
 * Disabled normally; bounded to128 real match samples when explicitly enabled. */
#ifndef MELEE_LB_BLEND_DIAGNOSTIC_H
#define MELEE_LB_BLEND_DIAGNOSTIC_H
#ifdef TARGET_PC
#include <math.h>
#include <stdlib.h>
#include <sysdolphin/baselib/jobj.h>
#include <dolphin/os.h>
extern unsigned int HSD_JObjDiagnosticMatchFrame;
typedef struct { double v[4]; } NativeBlendQuat;
typedef struct {
    double scale[3],translate[3],rotation[3][3];
    int armed,fast_euler,kind;
    float weight;
} NativeBlendDiagnostic;
static unsigned NativeBlendSamples;
static NativeBlendQuat NativeBlendMultiply(NativeBlendQuat a,NativeBlendQuat b)
{
    NativeBlendQuat q={{a.v[3]*b.v[0]+b.v[3]*a.v[0]+a.v[1]*b.v[2]-a.v[2]*b.v[1],
        a.v[3]*b.v[1]+b.v[3]*a.v[1]+a.v[2]*b.v[0]-a.v[0]*b.v[2],
        a.v[3]*b.v[2]+b.v[3]*a.v[2]+a.v[0]*b.v[1]-a.v[1]*b.v[0],
        a.v[3]*b.v[3]-a.v[0]*b.v[0]-a.v[1]*b.v[1]-a.v[2]*b.v[2]}};
    return q;
}
static NativeBlendQuat NativeBlendNormalize(NativeBlendQuat q)
{
    double norm=0;int i;for(i=0;i<4;++i)norm+=q.v[i]*q.v[i];norm=sqrt(norm);
    for(i=0;i<4;++i)q.v[i]/=norm;return q;
}
static NativeBlendQuat NativeBlendOrientation(Quaternion r,int is_quaternion)
{
    NativeBlendQuat q;
    if(is_quaternion){q=(NativeBlendQuat){{r.x,r.y,r.z,r.w}};return NativeBlendNormalize(q);}
    {
        NativeBlendQuat x={{sin((double)r.x/2),0,0,cos((double)r.x/2)}};
        NativeBlendQuat y={{0,sin((double)r.y/2),0,cos((double)r.y/2)}};
        NativeBlendQuat z={{0,0,sin((double)r.z/2),cos((double)r.z/2)}};
        return NativeBlendMultiply(NativeBlendMultiply(z,y),x);
    }
}
static NativeBlendQuat NativeBlendRelativeRotation(NativeBlendQuat a,NativeBlendQuat b,double t)
{
    NativeBlendQuat conjugate=a,relative,step;double dot=0,length,angle;int i;
    for(i=0;i<4;++i)dot+=a.v[i]*b.v[i];if(dot<0)for(i=0;i<4;++i)b.v[i]=-b.v[i];
    for(i=0;i<3;++i)conjugate.v[i]=-conjugate.v[i];
    relative=NativeBlendNormalize(NativeBlendMultiply(conjugate,b));
    length=sqrt(relative.v[0]*relative.v[0]+relative.v[1]*relative.v[1]+relative.v[2]*relative.v[2]);
    angle=atan2(length,relative.v[3])*t;
    for(i=0;i<3;++i)step.v[i]=length>1e-15?relative.v[i]*sin(angle)/length:0;
    step.v[3]=cos(angle);return NativeBlendNormalize(NativeBlendMultiply(a,step));
}
static void NativeBlendRotationMatrix(NativeBlendQuat q,double matrix[3][3])
{
    /* Rodrigues via a rotation vector, independently of C_MTXQuat. */
    double length=sqrt(q.v[0]*q.v[0]+q.v[1]*q.v[1]+q.v[2]*q.v[2]);
    double angle=2*atan2(length,q.v[3]),c=cos(angle),s=sin(angle),axis[3]={1,0,0};int r,k;
    if(length>1e-15)for(k=0;k<3;++k)axis[k]=q.v[k]/length;
    for(r=0;r<3;++r)for(k=0;k<3;++k)matrix[r][k]=(r==k?c:0)+(1-c)*axis[r]*axis[k];
    matrix[0][1]-=s*axis[2];matrix[0][2]+=s*axis[1];matrix[1][0]+=s*axis[2];
    matrix[1][2]-=s*axis[0];matrix[2][0]-=s*axis[1];matrix[2][1]+=s*axis[0];
}
static void NativeBlendReference(NativeBlendDiagnostic* state,const HSD_JObj* a,const HSD_JObj* b,float wa,float wb)
{
    double as[3]={a->scale.x,a->scale.y,a->scale.z},bs[3]={b->scale.x,b->scale.y,b->scale.z};
    double at[3]={a->translate.x,a->translate.y,a->translate.z},bt[3]={b->translate.x,b->translate.y,b->translate.z};
    NativeBlendQuat qa,qb;int i;
    for(i=0;i<3;++i){state->scale[i]=as[i]*wa+bs[i]*wb;state->translate[i]=at[i]*wa+bt[i]*wb;}
    state->fast_euler=!(a->flags&JOBJ_USE_QUATERNION)&&!(b->flags&JOBJ_USE_QUATERNION)&&
        fabsf(a->rotate.x-b->rotate.x)<=.0001f&&fabsf(a->rotate.y-b->rotate.y)<=.0001f&&fabsf(a->rotate.z-b->rotate.z)<=.0001f;
    qa=NativeBlendOrientation(a->rotate,a->flags&JOBJ_USE_QUATERNION);qb=NativeBlendOrientation(b->rotate,b->flags&JOBJ_USE_QUATERNION);
    NativeBlendRotationMatrix(state->fast_euler?qa:NativeBlendRelativeRotation(qa,qb,wb),state->rotation);
    state->weight=wb;
}
static int NativeBlendCompare(const NativeBlendDiagnostic* state,const HSD_JObj* output,double* maximum)
{
    double matrix[3][3],scale[3]={output->scale.x,output->scale.y,output->scale.z};
    double translation[3]={output->translate.x,output->translate.y,output->translate.z};int i,k,failures=0;
    NativeBlendRotationMatrix(NativeBlendOrientation(output->rotate,output->flags&JOBJ_USE_QUATERNION),matrix);
    *maximum=0;
    for(i=0;i<3;++i){
        double error=fabs(scale[i]-state->scale[i]);if(!isfinite(scale[i])||error>2e-5*(1+fabs(state->scale[i])))++failures;if(error>*maximum)*maximum=error;
        error=fabs(translation[i]-state->translate[i]);if(!isfinite(translation[i])||error>2e-5*(1+fabs(state->translate[i])))++failures;if(error>*maximum)*maximum=error;
        for(k=0;k<3;++k){error=fabs(matrix[i][k]-state->rotation[i][k]);if(!isfinite(matrix[i][k])||error>3e-4)++failures;if(error>*maximum)*maximum=error;}
    }
    if(!!(output->flags&JOBJ_USE_QUATERNION)==state->fast_euler)++failures;
    if(!(output->flags&JOBJ_MTX_DIRTY))++failures;
    return failures;
}
static int NativeBlendDiagnosticEnabled(void)
{
    static int enabled=-1;
    if(enabled<0)enabled=getenv("MELEE_DIAGNOSTIC_GAME_STARTUP")&&getenv("MELEE_DIAGNOSTIC_JOINT_BLEND");
    return enabled&&HSD_JObjDiagnosticMatchFrame>=300&&NativeBlendSamples<128;
}
static void NativeBlendBegin(NativeBlendDiagnostic* state,const HSD_JObj* a,const HSD_JObj* b,float wa,float wb,int kind)
{
    state->armed=NativeBlendDiagnosticEnabled();if(!state->armed)return;
    NativeBlendReference(state,a,b,wa,wb);state->kind=kind;
}
static void NativeBlendBeginDesc(NativeBlendDiagnostic* state,const HSD_Joint* a,const HSD_JObj* b,float wa,float wb)
{
    HSD_JObj rest;state->armed=NativeBlendDiagnosticEnabled();if(!state->armed)return;
    rest.flags=0;rest.rotate=(Quaternion){a->rotation.x,a->rotation.y,a->rotation.z,0};rest.scale=a->scale;rest.translate=a->position;
    NativeBlendReference(state,&rest,b,wa,wb);state->kind=1;
}
static void NativeBlendEnd(NativeBlendDiagnostic* state,const HSD_JObj* output)
{
    double maximum;int failures;if(!state->armed)return;
    failures=NativeBlendCompare(state,output,&maximum);++NativeBlendSamples;
    OSReport("NATIVE_JOINT_BLEND sample=%u frame=%u kind=%d weight=%f fast_euler=%d mismatches=%d max_error=%.9g joint=%p\n",NativeBlendSamples,HSD_JObjDiagnosticMatchFrame,state->kind,state->weight,state->fast_euler,failures,maximum,(const void*)output);
}
#endif
#endif
