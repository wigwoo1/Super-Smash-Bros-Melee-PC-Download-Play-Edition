/* Optional readonly skin-palette check. Reference transforms four points using
 * double arithmetic; inversion uses pivoted elimination, not SDK cofactors. */
#ifndef HSD_ENVELOPE_DIAGNOSTIC_H
#define HSD_ENVELOPE_DIAGNOSTIC_H
#ifdef TARGET_PC
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <dolphin/os.h>
#include <melee/ft/types.h>
#include <melee/pl/player.h>
#include <sysdolphin/baselib/gobj.h>
typedef double EnvelopeRefMtx[4][4];
static void EnvelopeRefCopy(Mtx input, EnvelopeRefMtx output)
{
    int r,c; memset(output,0,sizeof(EnvelopeRefMtx));
    for(r=0;r<3;r++)for(c=0;c<4;c++)output[r][c]=input[r][c];
    output[3][3]=1;
}
static void EnvelopeRefMultiply(EnvelopeRefMtx a,EnvelopeRefMtx b,EnvelopeRefMtx out)
{
    EnvelopeRefMtx tmp;int r,c,k;
    for(r=0;r<4;r++)for(c=0;c<4;c++){
        tmp[r][c]=0;for(k=0;k<4;k++)tmp[r][c]+=a[r][k]*b[k][c];
    }
    memcpy(out,tmp,sizeof(tmp));
}
static int EnvelopeRefInverse(EnvelopeRefMtx in,EnvelopeRefMtx out)
{
    double a[4][8],v;int r,c,k,p;
    for(r=0;r<4;r++)for(c=0;c<8;c++)a[r][c]=c<4?in[r][c]:(c-4==r);
    for(c=0;c<4;c++){
        p=c;for(r=c+1;r<4;r++)if(fabs(a[r][c])>fabs(a[p][c]))p=r;
        if(!(fabs(a[p][c])>1e-30))return 0;
        for(k=0;k<8;k++){v=a[c][k];a[c][k]=a[p][k];a[p][k]=v;}
        v=a[c][c];for(k=0;k<8;k++)a[c][k]/=v;
        for(r=0;r<4;r++)if(r!=c){v=a[r][c];for(k=0;k<8;k++)a[r][k]-=v*a[c][k];}
    }
    for(r=0;r<4;r++)for(c=0;c<4;c++)out[r][c]=a[r][c+4];return 1;
}
static void EnvelopeRefPoint(EnvelopeRefMtx a,const double* p,double* out)
{
    double q[4];int r,k;for(r=0;r<4;r++){q[r]=0;for(k=0;k<4;k++)q[r]+=a[r][k]*p[k];}
    memcpy(out,q,sizeof(q));
}
static int EnvelopeRefRight(HSD_JObj* owner,EnvelopeRefMtx out,int* present)
{
    HSD_JObj* skeleton;EnvelopeRefMtx a,b;int depth=0;
    *present=!(owner->flags&JOBJ_SKELETON_ROOT);if(!*present)return 1;
    for(skeleton=owner;skeleton&&!(skeleton->flags&(JOBJ_SKELETON|JOBJ_SKELETON_ROOT));skeleton=skeleton->parent)
        if(++depth>128)return 0;
    if(!skeleton)return 0;
    if(skeleton==owner){
        if(!owner->envelopemtx)return 0;
        EnvelopeRefCopy(owner->envelopemtx,a);return EnvelopeRefInverse(a,out);
    }
    EnvelopeRefCopy(skeleton->mtx,a);
    if(!(skeleton->flags&JOBJ_SKELETON_ROOT)){
        if(!skeleton->envelopemtx)return 0;
        EnvelopeRefCopy(skeleton->envelopemtx,b);EnvelopeRefMultiply(a,b,a);
    }
    if(!EnvelopeRefInverse(a,b))return 0;
    EnvelopeRefCopy(owner->mtx,a);EnvelopeRefMultiply(b,a,out);return 1;
}
static int EnvelopeRefPosition(HSD_JObj* owner,HSD_Envelope* head,Mtx view,
                               double output[3][4],unsigned* influences,int* has_right)
{
    EnvelopeRefMtx right,bone,bind,camera;double p[4],q[4],weighted[4],result[4],probes[4][3];
    HSD_Envelope* env;unsigned count=0;int i,r,c,single;
    if(!head||!EnvelopeRefRight(owner,right,has_right))return 0;
    single=head->weight>=(1.0f-FLT_EPSILON);
    for(env=head;env;env=single?NULL:env->next)if(++count>128)return 0;
    *influences=count;EnvelopeRefCopy(view,camera);
    for(i=0;i<4;i++){
        p[0]=p[1]=p[2]=0;p[3]=1;if(i)p[i-1]=1;
        if(*has_right)EnvelopeRefPoint(right,p,p);
        weighted[0]=weighted[1]=weighted[2]=0;weighted[3]=1;
        for(env=head;env;env=single?NULL:env->next){
            if(!env->jobj)return 0;memcpy(q,p,sizeof(q));
            if(!single||*has_right){
                if(!env->jobj->envelopemtx)return 0;
                EnvelopeRefCopy(env->jobj->envelopemtx,bind);EnvelopeRefPoint(bind,q,q);
            }
            EnvelopeRefCopy(env->jobj->mtx,bone);EnvelopeRefPoint(bone,q,q);
            for(r=0;r<3;r++)weighted[r]+=q[r]*(single?1:env->weight);
        }
        EnvelopeRefPoint(camera,weighted,result);for(r=0;r<3;r++)probes[i][r]=result[r];
    }
    for(r=0;r<3;r++){output[r][3]=probes[0][r];for(c=0;c<3;c++)output[r][c]=probes[c+1][r]-probes[0][r];}
    return 1;
}
static int EnvelopeRefNormal(Mtx position,double output[3][3])
{
    EnvelopeRefMtx a,inv;double det;int r,c;EnvelopeRefCopy(position,a);
    det=a[0][0]*(a[1][1]*a[2][2]-a[1][2]*a[2][1])-a[0][1]*(a[1][0]*a[2][2]-a[1][2]*a[2][0])+a[0][2]*(a[1][0]*a[2][1]-a[1][1]*a[2][0]);
    if(fabs(det)<1e-10){for(r=0;r<3;r++)for(c=0;c<3;c++)output[r][c]=a[r][c];return 1;}
    if(!EnvelopeRefInverse(a,inv))return 0;
    for(r=0;r<3;r++)for(c=0;c<3;c++)output[r][c]=inv[c][r];return 1;
}
extern unsigned int HSD_JObjDiagnosticMatchFrame;
static void PObjEnvelopeDiagnostic(HSD_PObj* pobj,HSD_JObj* owner,Mtx view,
                                    Mtx position,Mtx normal,int slot,int has_normal)
{
    static int enabled=-1;static unsigned count,bucket=~0u,per_player[4][2];
    HSD_SList* list;double expected[3][4],n[3][3],err,maximum=0;unsigned influences=0;
    int i,r,c,right=0,valid,failures=0,kind=-1,depth,player=-1;HSD_JObj* ancestor;
    if(enabled<0)enabled=getenv("MELEE_DIAGNOSTIC_GAME_STARTUP")&&getenv("MELEE_DIAGNOSTIC_ENVELOPE");
    if(!enabled||HSD_JObjDiagnosticMatchFrame<300||count>=256)return;
    /* Only accept a current fighter's original model hierarchy, not stage
     * envelopes that happen to be rendered in the same frame. */
    for(i=0;i<4&&kind<0;i++){
        HSD_GObj* entity=Player_GetEntity(i);Fighter* fighter=entity?entity->user_data:NULL;
        if(!fighter)continue;
        for(ancestor=owner,depth=0;ancestor&&depth<128;ancestor=ancestor->parent,depth++)
            if(ancestor==(HSD_JObj*)entity->hsd_obj){kind=fighter->kind;player=i;break;}
    }
    if(kind<0)return;
    if(bucket!=HSD_JObjDiagnosticMatchFrame/8){bucket=HSD_JObjDiagnosticMatchFrame/8;memset(per_player,0,sizeof(per_player));}
    if(per_player[player][has_normal]!=0 && per_player[player][has_normal]>=2)return;
    per_player[player][has_normal]++;
    list=pobj->u.envelope_list;for(i=0;i<slot&&list;i++)list=list->next;
    valid=list&&EnvelopeRefPosition(owner,list->data,view,expected,&influences,&right);
    if(valid)for(r=0;r<3;r++)for(c=0;c<4;c++){
        err=fabs(position[r][c]-expected[r][c]);if(!(err<=2e-4*(1+fabs(expected[r][c]))))failures++;
        if(err>maximum)maximum=err;
    }
    if(has_normal){valid=valid&&EnvelopeRefNormal(position,n);if(valid)for(r=0;r<3;r++)for(c=0;c<3;c++){
        err=fabs(normal[r][c]-n[r][c]);if(!(err<=2e-4*(1+fabs(n[r][c]))))failures++;
        if(err>maximum)maximum=err;
    }}
    if(!valid)failures++;
    OSReport("NATIVE_ENVELOPE sample=%u frame=%u actor=%d slot=%d influences=%u right=%d normal=%d valid=%d mismatches=%d max_error=%.9g\n",
        ++count,HSD_JObjDiagnosticMatchFrame,kind,slot,influences,right,has_normal,valid,failures,maximum);
}
#endif
#endif
