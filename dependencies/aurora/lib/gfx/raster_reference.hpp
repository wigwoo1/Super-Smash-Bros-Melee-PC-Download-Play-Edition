#pragma once
#include <array>
#include <cmath>
namespace aurora::gfx::raster_reference {
inline bool zeroArea(const std::array<std::array<double,4>,3>& clip) noexcept {
  double x[3],y[3];
  for(unsigned i=0;i<3;++i) {
    for(double v:clip[i])if(!std::isfinite(v))return false;
    if(clip[i][3]<=0 || clip[i][2]<0 || clip[i][2]>clip[i][3])return false;
    x[i]=clip[i][0]/clip[i][3];y[i]=clip[i][1]/clip[i][3];
    if(!std::isfinite(x[i]) || !std::isfinite(y[i]))return false;
  }
  const double area=(x[1]-x[0])*(y[2]-y[0])-(y[1]-y[0])*(x[2]-x[0]);
  return std::isfinite(area) && area==0.; // No epsilon: near-zero remains unknown.
}
// Conservative exclusion only: invalid input and the edge band return false.
// A negative barycentric coordinate below -0.02 proves no triangle coverage.
inline bool definitelyOutside(const std::array<std::array<double,4>,3>& clip,
                              double px,double py) noexcept {
  if(!std::isfinite(px) || !std::isfinite(py))return false;
  double x[3],y[3];
  for(unsigned i=0;i<3;++i) {
    for(double v:clip[i])if(!std::isfinite(v))return false;
    if(clip[i][3]<=0 || clip[i][2]<0 || clip[i][2]>clip[i][3])return false;
    x[i]=(clip[i][0]/clip[i][3]+1)*128;y[i]=(1-clip[i][1]/clip[i][3])*128;
    if(!std::isfinite(x[i]) || !std::isfinite(y[i]))return false;
  }
  const double d=(y[1]-y[2])*(x[0]-x[2])+(x[2]-x[1])*(y[0]-y[2]);
  if(!std::isfinite(d) || std::abs(d)<1e-8)return false;
  const double a=((y[1]-y[2])*(px-x[2])+(x[2]-x[1])*(py-y[2]))/d;
  const double b=((y[2]-y[0])*(px-x[2])+(x[0]-x[2])*(py-y[2]))/d,c=1-a-b;
  return std::isfinite(a) && std::isfinite(b) && std::isfinite(c) && (a<-.02 || b<-.02 || c<-.02);
}
// WebGPU framebuffer Y points down: positive signed area is clockwise.
// Reject clipped/invalid input; clipping can change the polygon and requires a separate oracle.
inline bool clockwise(const std::array<std::array<double,4>,3>& clip, bool& result) noexcept {
  double x[3], y[3];
  for(unsigned i=0;i<3;++i) {
    for(double v:clip[i]) if(!std::isfinite(v)) return false;
    if(clip[i][3]<=0 || clip[i][2]<0 || clip[i][2]>clip[i][3]) return false;
    x[i]=clip[i][0]/clip[i][3]; y[i]=-clip[i][1]/clip[i][3];
    if(!std::isfinite(x[i]) || !std::isfinite(y[i])) return false;
  }
  const double area=(x[1]-x[0])*(y[2]-y[0])-(y[1]-y[0])*(x[2]-x[0]);
  if(!std::isfinite(area) || std::abs(area)<1e-12) return false;
  result=area>0; return true;
}
// Pixel-centre reference for a triangle with positive clip W. Reject outside,
// degenerate and clipped-depth samples rather than inventing expected pixels.
inline bool weights(const std::array<std::array<double,4>,3>& clip,
                    double width,double height,double px,double py,
                    std::array<double,3>& perspective,double& depth) noexcept {
  if(!std::isfinite(width) || !std::isfinite(height) || width<=0 || height<=0 ||
     !std::isfinite(px) || !std::isfinite(py) || px<0 || py<0 || px>=width || py>=height) return false;
  double x[3],y[3];
  for(unsigned i=0;i<3;++i) {
    for(double c:clip[i]) if(!std::isfinite(c)) return false;
    if(clip[i][3]<=0) return false;
    x[i]=(clip[i][0]/clip[i][3]+1)*width/2;
    y[i]=(1-clip[i][1]/clip[i][3])*height/2;
    if(!std::isfinite(x[i]) || !std::isfinite(y[i])) return false;
  }
  const double determinant=(y[1]-y[2])*(x[0]-x[2])+(x[2]-x[1])*(y[0]-y[2]);
  if(!std::isfinite(determinant) || std::abs(determinant)<1e-8) return false;
  std::array<double,3> screen{
    ((y[1]-y[2])*(px-x[2])+(x[2]-x[1])*(py-y[2]))/determinant,
    ((y[2]-y[0])*(px-x[2])+(x[0]-x[2])*(py-y[2]))/determinant,0};
  screen[2]=1-screen[0]-screen[1];
  double denominator=0,z=0;
  std::array<double,3> result{};
  for(unsigned i=0;i<3;++i) {
    if(!std::isfinite(screen[i]) || screen[i]<0.02) return false; // Stay away from edge coverage rules.
    result[i]=screen[i]/clip[i][3];denominator+=result[i];
    z+=screen[i]*clip[i][2]/clip[i][3];
  }
  if(!std::isfinite(denominator) || denominator<=0 || !std::isfinite(z) || z<0 || z>1) return false;
  for(double& value:result) value/=denominator;
  perspective=result;depth=z;return true;
}
}
