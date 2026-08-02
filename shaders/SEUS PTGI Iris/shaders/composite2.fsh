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

#include "lib/Uniforms.inc"
#include "lib/Common.inc"

in vec4 texcoord;
in vec3 lightVector;

in float timeSunriseSunset;
in float timeNoon;
in float timeMidnight;
in float timeSkyDark;

in vec3 colorSunlight;
in vec3 colorSkylight;
in vec3 colorSunglow;
in vec3 colorBouncedSunlight;
in vec3 colorScatteredSunlight;
in vec3 colorTorchlight;
in vec3 colorWaterMurk;
in vec3 colorWaterBlue;
in vec3 colorSkyTint;

in vec3 upVector;

#include "lib/GBufferData.inc"

#include "lib/VoxelTracing.inc"
#include "lib/GIHelpers.inc"

// Compatibility wrappers for deobfuscated call sites
int d() { return GetScreenVolumeSide(); }
int f() { return GetShadowVolumeSide(); }
vec3 n(vec2 v) { return ScreenUVToVolumeCoord(v); }
vec2 s(vec3 v) { return VolumeCoordToScreenUV(v); }
vec3 x(vec2 v) { return ShadowUVToVolumeCoord(v); }
vec2 d(vec3 v,int y) { return VolumeCoordToShadowUV(v, y); }
vec3 f(vec3 v,int y) { return WorldOffsetToVolumeCoordClamped(v, y); }
vec3 n(vec3 v,int y) { return WorldOffsetToVolumeCoord(v, y); }

 vec3 v(vec3 v) { return VolumeCoordToShadowWorldOffset(v); }
vec3 e(vec3 v) { return WorldOffsetToScreenVolumeCoord(v); }
vec3 r(vec3 v) { return ScreenVolumeCoordToWorldOffset(v); }
vec3 e() { return GetCameraVoxelDelta(); }
vec3 m(vec3 v) { return WorldPosToShadowMapCoord(v); }
vec3 d(vec3 v,vec3 f,vec2 n,vec2 y,vec4 i,vec4 m,inout float x,out vec2 z) { return AdjustVoxelHitForBlockShape(v, f, n, y, i, m, x, z); }
VoxelDDA p(Ray v) { return InitVoxelDDA(v); }
void w(inout VoxelDDA v) { StepVoxelDDA(v); }
void d(in Ray v,in vec3 f[2],out float i,out float y) { RayAABBIntersectDistances(v, f, i, y); }
vec3 d(const vec3 v,const vec3 f,vec3 y) { return AABBSurfaceNormal(v, f, y); }
bool e(const vec3 v,const vec3 f,Ray m,out vec2 i) { return RayAABBIntersect(v, f, m, i); }
bool d(const vec3 v,const vec3 f,Ray m,inout float x,inout vec3 y) { return RayAABBHitCloser(v, f, m, x, y); }
vec3 e(vec3 v,vec3 f,vec3 y,vec3 x,int z) { return SampleShadowedSunlightWithCaustics(v, f, y, x, z); }
vec3 f(vec3 y,vec3 f,vec3 x,vec3 z,int n) { return SampleShadowedSunlightStained(y, f, x, z, n); }
vec3 m(vec3 v,vec3 f,vec3 y,vec3 x,int z) { return SampleShadowedSunlightStained(v, f, y, x, z); }
vec4 i(GIBufferData v) { return PackGIBufferData(v); }
GIBufferData h(vec4 v) { return UnpackGIBufferData(v); }
GIBufferData R(vec2 v) { return SampleGIBuffer(v); }
float R(float v,float y) { return ReflectionStrengthFromSmoothness(v, y); }
bool R(vec3 v,float y,Ray f,bool x,inout float i,inout vec3 z) { return TraceBlockShape(v, y, f, x, i, z); }

 float e(float v,float y)
 {
   return exp(-pow(v/(.9*y),2.));
 }
 float h(vec3 v,vec3 y)
 {
   return dot(abs(v-y),vec3(.3333));
 }
 vec3 z(vec2 uv) { return SampleBlueNoise(uv); }
 vec3 R(float v,float f,float y,vec3 i)
 {
   vec3 m;
   m.x=y*cos(v);
   m.y=y*sin(v);
   m.z=f;
   vec3 s=abs(i.y)<.999?vec3(0,0,1):vec3(1,0,0),x=normalize(cross(i,vec3(0.,1.,1.))),c=cross(x,i);
   return x*m.x+c*m.y+i*m.z;
 }
 vec3 R(vec2 v,float y,vec3 z)
 {
   float f=2*3.14159*v.x,x=sqrt((1-v.y)/(1+(y*y-1)*v.y)),s=sqrt(1-x*x);
   return R(f,x,s,z);
 }
 float G(float v)
 {
   return 2./(v*v+1e-07)-2.;
 }
 vec3 G(in vec2 v,in float y,in vec3 z)
 {
   float f=G(y),i=2*3.14159*v.x,x=pow(v.y,1.f/(f+1.f)),s=sqrt(1-x*x);
   return R(i,x,s,z);
 }
 float a(vec2 v)
 {
   return texture2DLod(colortex3,v,0).w;
 }
 float G(float v,float y)
 {
   return v/(y*20.01+1.);
 }
 vec2 a(vec2 v,float y)
 {
   vec2 s=v;
   mat2 x=mat2(cos(y),-sin(y),sin(y),cos(y));
   v=x*v;
   return v;
 }
 vec4 G(sampler2D v,float f,bool y,float i,float s,float z,float x)
 {
   GBufferData m=GetGBufferData();
   GBufferDataTransparent n=GetGBufferDataTransparent();
   bool r=n.depth<m.depth;
   if(r)
     m.normal=n.normal,m.smoothness=n.smoothness,m.metalness=0.,m.mcLightmap=n.mcLightmap,m.depth=n.depth;
   vec4 c=GetViewPosition(texcoord.xy,m.depth),w=gbufferModelViewInverse*vec4(c.xyz,1.),d=gbufferModelViewInverse*vec4(c.xyz,0.);
   vec3 t=normalize(c.xyz),o=normalize(d.xyz),A=normalize((gbufferModelViewInverse*vec4(m.normal,0.)).xyz);
   float p=GetDepthLinear(texcoord.xy),l=dot(-t,m.normal.xyz),e=1.-m.smoothness,M=e*e,b=R(m.smoothness,m.metalness);
   vec4 W=texture2DLod(colortex6,texcoord.xy,0);
   float Y=Luminance(W.xyz);
   if(b<.001)
     return W;
   float g=f*.9;
   g*=min(M*20.,1.1);
   g*=mix(W.w,1.,1.);
   vec2 D=vec2(0.);
   if(y)
     {
       vec2 T=BlueNoiseTemporal(texcoord.xy).xy*.99+.005;
       D=T-.5;
     }
   float T=0.,S=1.1,u=G(i,m.totalTexGrad)/(M+.0001),L=G(s,m.totalTexGrad);
   vec4 P=vec4(0.),Q=vec4(0.);
   float U=0.;
   vec4 H=vec4(vec3(z),1.);
   H.xyz=vec3(.5);
   H.xyz*=W.w*.95+.05;
   float J=m.smoothness;
   vec2 F=normalize(cross(m.normal,t).xy),k=a(F,1.5708);
   float K=1.-pow(1.-saturate(l),1.);
   F*=mix(.1675,.5,K);
   k*=mix(mix(.7,.7,M),.5,K);
   vec3 N=reflect(-t,m.normal);
   int B=0;
   for(int I=-1;
I<=1;
I++)
     {
       for(int V=-1;
V<=1;
V++)
         {
           vec2 q=vec2(I,V)+D;
           q=q.x*F+q.y*k;
           q*=g*1.5/vec2(viewWidth,viewHeight);
           vec2 E=texcoord.xy+q.xy;
           float X=length(q*vec2(viewWidth,viewHeight));
           if(X*.025>W.w+.1)
             {
               continue;
             }
           E=clamp(E,4./vec2(viewWidth,viewHeight),1.-4./vec2(viewWidth,viewHeight));
           vec4 O=texture2DLod(colortex6,E,0);
           vec3 C=GetNormals(E);
           float j=GetDepthLinear(E),Z=pow(saturate(dot(N,reflect(-t,C))),105./M),ab=exp(-(abs(j-p)*S)),ac=exp(-(h(O.xyz,W.xyz)*T)),ad=exp(-abs(J-a(E))*L),ae=Z*ab*ac*ad;
           P+=vec4(pow(length(O.xyz),H.x)*normalize(O.xyz+1e-05),O.w)*ae;
           U+=ae;
           Q+=O;
           B++;
         }
     }
   P/=U+.0001;
   P.xyz=pow(length(P.xyz),1./H.x)*normalize(P.xyz+1e-06);
   vec4 q=P;
   if(U<.001)
     q=W;
   return q;
 }
 void main()
 {
   vec4 v=texture2DLod(colortex6,texcoord.xy,4);
   vec3 y=pow(texture2DLod(colortex3,texcoord.xy,2).xyz,vec3(2.2)),s=GetViewPosition(texcoord.xy,GetDepth(texcoord.xy)).xyz,z=GetNormals(texcoord.xy);
   float m=pow(1.-saturate(dot(-normalize(s),z)),5.);
   v.xyz*=m;
   float x=dot(max(vec3(0.),vec3(v.xyz-y.xyz*20.)),vec3(240.));
   vec4 f=texture2DLod(colortex6,texcoord.xy,0);
   f=G(colortex6,15.,false,180.,40.,.1,0.);
   gl_FragData[0]=vec4(f);
 }

/* RENDERTARGETS: 6 */
