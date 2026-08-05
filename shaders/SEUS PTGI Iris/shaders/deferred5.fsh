#version 330 compatibility

/*
 _______ _________ _______  _______  _ 
(  ____ \\__   __/(  ___  )(  ____ )( )
| (    \/   ) (   | (   ) || (    )|| |
| (_____    | |   | |   | || (____)|| |
(_____  )   | |   | |   | ||  _____)| |
      ) |   | |   | |   | || (      (_)
/\____) |   | |   | (___) || )       _ 
\_______)   )_(   (_______)|/       (_)

Do not modify this code until you have read the LICENSE.txt contained in the root directory of this shaderpack!

*/

in vec4 texcoord;

#include "lib/Uniforms.inc"
#include "lib/Common.inc"
#include "lib/GBufferData.inc"

#include "lib/VoxelTracing.inc"

// Compatibility wrappers for deobfuscated call sites
int d() { return GetScreenVolumeSide(); }
int f() { return GetShadowVolumeSide(); }
vec3 x(vec2 v) { return ScreenUVToVolumeCoord(v); }
vec2 n(vec3 v) { return VolumeCoordToScreenUV(v); }
vec3 s(vec2 v) { return ShadowUVToVolumeCoord(v); }
vec2 d(vec3 v,int y) { return VolumeCoordToShadowUV(v, y); }
vec3 f(vec3 v,int y) { return WorldOffsetToVolumeCoordClamped(v, y); }
vec3 n(vec3 v,int y) { return WorldOffsetToVolumeCoord(v, y); }

 vec3 v(vec3 v) { return VolumeCoordToShadowWorldOffset(v); }
vec3 r(vec3 v) { return WorldOffsetToScreenVolumeCoord(v); }
vec3 p(vec3 v) { return ScreenVolumeCoordToWorldOffset(v); }
vec3 n() { return GetCameraVoxelDelta(); }
vec3 m(vec3 v) { return WorldPosToShadowMapCoord(v); }
vec3 d(vec3 v,vec3 i,vec2 n,vec2 f,vec4 x,vec4 m,inout float y,out vec2 s) { return AdjustVoxelHitForBlockShape(v, i, n, f, x, m, y, s); }
VoxelDDA e(Ray v) { return InitVoxelDDA(v); }
void w(inout VoxelDDA v) { StepVoxelDDA(v); }
bool e(const vec3 v,const vec3 i,Ray m,out vec2 n) { return RayAABBIntersect(v, i, m, n); }
bool d(const vec3 v,const vec3 i,Ray m,inout float x,inout vec3 y) { return RayAABBHitCloser(v, i, m, x, y); }
vec3 e(vec3 v,vec3 i,vec3 y,vec3 x,int z) { return SampleShadowedSunlightWithCaustics(v, i, y, x, z); }
vec3 f(vec3 y,vec3 i,vec3 x,vec3 z,int n) { return SampleShadowedSunlightStained(y, i, x, z, n); }
vec3 m(vec3 v,vec3 i,vec3 y,vec3 x,int z) { return SampleShadowedSunlightStained(v, i, y, x, z); }
vec4 h(GIBufferData v) { return PackGIBufferData(v); }
GIBufferData i(vec4 v) { return UnpackGIBufferData(v); }
GIBufferData G(vec2 v) { return SampleGIBuffer(v); }
float G(float v,float y) { return ReflectionStrengthFromSmoothness(v, y); }
bool G(vec3 v,float y,Ray i,bool x,inout float f,inout vec3 z) { return TraceBlockShape(v, y, i, x, f, z); }

 void d(inout float v,inout float y,float i,float f,vec3 x,float z)
 {
   #if GI_FILTER_QUALITY==0
   v*=mix(2.4,2.6,f);
   #else
   v*=mix(2.4,2.9,f);
   #endif
   float h=dot(x,vec3(1.));
   y*=1.-pow(f,.4);
   y/=i*.1+2e-06;
   y*=.35;
   if(z<.12)
     y=0.;
 }
 float G(vec3 v,vec3 y,float x)
 {
   float i=dot(abs(v-y),vec3(.3333));
   i*=x;
   i*=.18;
   return i;
 }
 vec4 G(sampler2D v,vec2 i,bool y,float x,float f,vec2 h,out float r)
 {
   GIBufferData m=G(i.xy);
   r=m.auxG;
   vec4 n=texture2DLod(v,i.xy,0);
   vec3 s=n.xyz;
   float z=n.w;
   vec3 a=GetNormals(i.xy);
   float w=GetDepth(i.xy),t=ExpToLinearDepth(w);
   vec3 e=GetViewPosition(i.xy,w).xyz;
   f=pow(f,2.);
   vec2 M=vec2(0.);
   if(y)
     M=BlueNoiseTemporal(i.xy).xy-.5;
   float c=x*1,o=f;
   d(c,o,n.w,r,s,t);
   float R=24.*mix(4.,4.,r),A=mix(20.,10.,r)/t,Y=0.;
   vec4 b=vec4(0.);
   float p=0.;
   int W=0;
   for(int l=-1;
l<=1;
l+=1)
     {
       {
         vec2 D=vec2(l+M.x)/vec2(viewWidth,viewHeight)*c*h,g=i.xy+D.xy;
         float H=length(D*vec2(viewWidth,viewHeight));
         g=clamp(g,4./vec2(viewWidth,viewHeight),1.-4./vec2(viewWidth,viewHeight));
         vec4 T=texture2DLod(v,g,0);
         vec3 S=GetNormals(g);
         float J=GetDepth(g),u=ExpToLinearDepth(J),L=pow(saturate(dot(a,S)),R),P=exp(-(abs(u-t)*A)),Q=0.;
         vec3 F=GetViewPosition(g,J).xyz,k=normalize(F.xyz-e.xyz);
         float U=dot(-S,k);
         bool E=U>0.&&Luminance(T.xyz)<Luminance(s.xyz);
         L=E?1.:L;
         Q=exp(-G(T.xyz,s,E?o:o));
         float B=P*Q*L;
         b+=T*B;
         p+=B;
         W++;
       }
     }
   b/=p+.0001;
   if(p<.0001)
     b=n;
   return b;
 }
 float e(float v,float y)
 {
   return exp(-pow(v/(.9*y),2.));
 }
 float h(vec3 v,vec3 y)
 {
   return dot(abs(v-y),vec3(.3333));
 }
 void main()
 {
   float v;
   vec4 y=G(colortex6,texcoord.xy,true,2.,2.,vec2(0.,1.),v);
   gl_FragData[0]=y;
 }

/* RENDERTARGETS: 6 */
