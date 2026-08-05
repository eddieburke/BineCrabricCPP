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
bool e(const vec3 v,const vec3 i,Ray m,out vec2 f) { return RayAABBIntersect(v, i, m, f); }
bool d(const vec3 v,const vec3 i,Ray m,inout float x,inout vec3 y) { return RayAABBHitCloser(v, i, m, x, y); }
vec3 e(vec3 v,vec3 i,vec3 y,vec3 x,int z) { return SampleShadowedSunlightWithCaustics(v, i, y, x, z); }
vec3 f(vec3 y,vec3 i,vec3 x,vec3 f,int z) { return SampleShadowedSunlightStained(y, i, x, f, z); }
vec3 m(vec3 v,vec3 i,vec3 y,vec3 x,int z) { return SampleShadowedSunlightStained(v, i, y, x, z); }
vec4 i(GIBufferData v) { return PackGIBufferData(v); }
GIBufferData h(vec4 v) { return UnpackGIBufferData(v); }
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
   float c=dot(x,vec3(1.));
   y*=1.-pow(f,.4);
   y/=i*.1+2e-06;
   y*=.35;
   if(z<.12)
     y=0.;
 }
 float G(vec3 v,vec3 y,float m)
 {
   float i=dot(abs(v-y),vec3(.3333));
   i*=m;
   i*=.18;
   return i;
 }
 vec4 G(sampler2D v,vec2 i,bool y,float x,float f,vec2 c,out float r)
 {
   GIBufferData m=G(i.xy);
   r=m.auxG;
   vec4 n=texture2DLod(v,i.xy,0);
   vec3 s=n.xyz;
   float z=n.w;
   vec3 h=GetNormals(i.xy);
   float t=GetDepth(i.xy),w=ExpToLinearDepth(t);
   vec3 e=GetViewPosition(i.xy,t).xyz;
   f=pow(f,2.);
   vec2 M=vec2(0.);
   if(y)
     M=BlueNoiseTemporal(i.xy).xy-.5;
   float a=x*1,o=f;
   d(a,o,n.w,r,s,w);
   float R=24.*mix(4.,4.,r),A=mix(20.,10.,r)/w,Y=0.;
   vec4 b=vec4(0.);
   float p=0.;
   int W=0;
   for(int F=-1;
F<=1;
F+=1)
     {
       {
         vec2 l=vec2(F+M.x)/vec2(viewWidth,viewHeight)*a*c,g=i.xy+l.xy;
         float D=length(l*vec2(viewWidth,viewHeight));
         g=clamp(g,4./vec2(viewWidth,viewHeight),1.-4./vec2(viewWidth,viewHeight));
         vec4 T=texture2DLod(v,g,0);
         vec3 H=GetNormals(g);
         float B=GetDepth(g),u=ExpToLinearDepth(B),S=pow(saturate(dot(h,H)),R),E=exp(-(abs(u-w)*A)),L=0.;
         vec3 J=GetViewPosition(g,B).xyz,Q=normalize(J.xyz-e.xyz);
         float P=dot(-H,Q);
         bool U=P>0.&&Luminance(T.xyz)<Luminance(s.xyz);
         S=U?1.:S;
         L=exp(-G(T.xyz,s,U?o:o));
         float K=E*L*S;
         b+=T*K;
         p+=K;
         W++;
       }
     }
   b/=p+.0001;
   if(p<.0001)
     b=n;
   return b;
 }
 float c(vec3 v)
 {
   float y=simplex3d(v*vec3(1.,.01,1.5)+vec3(-v.z*1.2+v.y*.2,0.,0.));
   y+=simplex3d(v*vec3(1.,.01,.9)+vec3(-v.z*.7-v.y*.2,0.,0.));
   return y*1.5;
 }
 vec2 c(vec2 v,float y)
 {
   v*=8.;
   float i=c(vec3(v.xy*5.+vec2(-.025,-.025),y))-.5,f=c(vec3(v.xy*5.+vec2(.025,-.025),y))-.5,x=c(vec3(v.xy*5.+vec2(-.025,.025),y))-.5;
   vec3 n=normalize(vec3(i-f,i-x,2.5));
   return n.xy;
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
   vec4 i=G(colortex6,texcoord.xy,false,8.,8.,vec2(0.,1.),v);
   float y=1.;
   if(texcoord.y<.25)
     {
       vec2 n=texcoord.xy*vec2(4.,4.),f=vec2(n.x,(n.y-floor(mod(FRAME_TIME*60.f,60.f)))/60.f);
       if(texcoord.x<.25)
         y=texture2DLod(colortex0,f.xy,0).x;
       else
          if(texcoord.x>.25&&texcoord.x<.5)
           y=texture2DLod(colortex0,f.xy,0).y;
         else
            if(texcoord.x>.5&&texcoord.x<.75)
             y=texture2DLod(colortex0,f.xy,0).z;
           else
              y=texture2DLod(colortex0,f.xy,0).w;
     }
   vec2 f=1.-abs(texcoord.xy*2.-1.);
   f=saturate(f*10.);
   float x=min(f.x,f.y);
   i.xyz*=mix(vec3(1.),BlueNoiseTemporal(texcoord.xy).xyz*2.,vec3(1.-pow(v,2.5))*.25*x);
   gl_FragData[0]=vec4(i.xyz,y);
 }

/* RENDERTARGETS: 6 */
