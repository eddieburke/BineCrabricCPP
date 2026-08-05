#version 330 compatibility

#include "lib/Uniforms.inc"
#include "lib/Common.inc"

in vec4 mc_Entity;
in vec4 at_tangent;
in vec4 mc_midTexCoord;

out vec4 vTexcoord;
out vec4 vColor;
// out vec4 lmcoord;

// out vec3 normal;
out vec4 vViewPos;
out float vMaterialIDs;

out float vMCEntity;
out float vIsWater;

out float invalidForVolume;
out float unusedShadowFlag;
out float vFragDepth;
out vec2 vMidTexCoord;

out vec4 volumePosition;
out vec4 shadowPosition;

#include "lib/VoxelTracing.inc"

// Compatibility wrappers for deobfuscated call sites
int d() { return GetScreenVolumeSide(); }
int f() { return GetShadowVolumeSide(); }
vec3 n(vec2 v) { return ScreenUVToVolumeCoord(v); }
vec2 x(vec3 v) { return VolumeCoordToScreenUV(v); }
vec3 v(vec2 v) { return ShadowUVToVolumeCoord(v); }
vec2 d(vec3 v,int side) { return VolumeCoordToShadowUV(v, side); }
vec3 f(vec3 v,int y) { return WorldOffsetToVolumeCoordClamped(v, y); }
vec3 n(vec3 v,int y) { return WorldOffsetToVolumeCoord(v, y); }
vec3 m() { return GetCameraVoxelDelta(); }
vec3 i(vec3 v) { return WorldPosToShadowMapCoord(v); }

 vec3 d(vec3 v,vec3 m,vec2 x,vec2 y,vec4 n,vec4 f,inout float i,out vec2 s) { return AdjustVoxelHitForBlockShape(v, m, x, y, n, f, i, s); }
VoxelDDA p(Ray v) { return InitVoxelDDA(v); }
void e(inout VoxelDDA v) { StepVoxelDDA(v); }
bool e(const vec3 v,const vec3 m,Ray f,out vec2 i) { return RayAABBIntersect(v, m, f, i); }
bool d(const vec3 v,const vec3 x,Ray f,inout float y,inout vec3 z) { return RayAABBHitCloser(v, x, f, y, z); }
vec3 e(vec3 v,vec3 m,vec3 y,vec3 x,int f) { return SampleShadowedSunlightWithCaustics(v, m, y, x, f); }
vec3 f(vec3 v,vec3 x,vec3 y,vec3 f,int z) { return SampleShadowedSunlightVolume(v, x, y, f, z); }
vec3 i(vec3 v,vec3 m,vec3 y,vec3 x,int z) { return SampleShadowedSunlightStained(v, m, y, x, z); }
vec4 w(GIBufferData v) { return PackGIBufferData(v); }
GIBufferData G(vec4 v) { return UnpackGIBufferData(v); }
GIBufferData h(vec2 v) { return SampleGIBuffer(v); }
float G(float v,float y) { return ReflectionStrengthFromSmoothness(v, y); }
bool G(vec3 v,float x,Ray y,bool m,inout float f,inout vec3 i) { return TraceBlockShape(v, x, y, m, f, i); }

 vec4 G(vec3 x,vec2 v,out float i,inout float z)
 {
   vec3 y=x;
   int n=f();
   x=clamp(x,vec3(1./n),vec3(1.-1./n));
   if(distance(x,y)>.005/n)
     z=1.;
   float s=dot(abs(x-y),vec3(1.));
   #ifdef MC_GL_VENDOR_ATI
   x-=1./float(n)*vec3(0.,0.,1.);
   #endif
   vec2 r=d(x,n);
   r+=v.xy*(1./vec2(4096));
   r=r*2.-1.;
   i=s;
   return vec4(r,i,1.);
 }
 void main()
 {
   gl_Position=ftransform();
   vTexcoord=gl_MultiTexCoord0;
   vMCEntity=mc_Entity.x;
   vViewPos=gl_ModelViewMatrix*gl_Vertex;
   vec4 v=gl_Position;
   v=shadowProjectionInverse*v;
   v=shadowModelViewInverse*v;
   v.xyz+=cameraPosition.xyz;
   vec3 x=v.xyz;
   vMaterialIDs=30.;
   float z=0.,s=0.f,i=0.f;
   vIsWater=0.;
   if(mc_Entity.x==8||mc_Entity.x==9)
     {
       z=1.f;
       vIsWater=1.f;
     }
   if(mc_Entity.x==95||mc_Entity.x==160||mc_Entity.x==90||mc_Entity.x==165||mc_Entity.x==79)
     i=1.f;
   if(mc_Entity.x==79)
     s=1.f;
   if(mc_Entity.x==18.||mc_Entity.x==161.f)
     vMaterialIDs=36.;
   if(mc_Entity.x==79.f||mc_Entity.x==174.f)
     vMaterialIDs=37.;
   if(mc_Entity.x==50)
     vMaterialIDs=241.;
   if(mc_Entity.x==76)
     vMaterialIDs=241.;
   if(mc_Entity.x==10||mc_Entity.x==11)
     vMaterialIDs=241.;
   if(mc_Entity.x==89||mc_Entity.x==124||mc_Entity.x==10||mc_Entity.x==11||mc_Entity.x==169||mc_Entity.x==91)
     vMaterialIDs=31.;
   if(mc_Entity.x==51)
     vMaterialIDs=241.;
   #ifdef GLOWING_LAPIS_LAZULI_BLOCK
   if(mc_Entity.x==22)
     vMaterialIDs=31.;
   #endif
   #ifdef GLOWING_REDSTONE_BLOCK
   if(mc_Entity.x==152)
     vMaterialIDs=31.;
   #endif
   #ifdef GLOWING_EMERALD_BLOCK
   if(mc_Entity.x==133)
     vMaterialIDs=31.;
   #endif
   if(mc_Entity.x==95||mc_Entity.x==160)
     vMaterialIDs=240.;
   if(mc_Entity.x==188)
     vMaterialIDs=32.;
   if(mc_Entity.x==189)
     vMaterialIDs=33.;
   if(mc_Entity.x==190)
     vMaterialIDs=34.;
   if(mc_Entity.x==191)
     vMaterialIDs=35.;
   vec3 y=gl_Normal,r=normalize(gl_NormalMatrix*y);
   if(abs(vMaterialIDs-2.)<.1)
     y=vec3(0.,1.,0.);
   vMidTexCoord=mc_midTexCoord.xy;
   vColor=gl_Color;
   if(vMaterialIDs!=2.)
     {
       if(y.x>.85)
         vColor.xyz*=1./.6;
       if(y.x<-.85)
         vColor.xyz*=1./.6;
       if(y.z>.85)
         vColor.xyz*=1.25;
       if(y.z<-.85)
         vColor.xyz*=1.25;
       if(y.y<-.85)
         vColor.xyz*=2.;
     }
   invalidForVolume=0.;
   {
     vec2 m;
     vec3 a=d(v.xyz,gl_Normal.xyz,vTexcoord.xy,mc_midTexCoord.xy,at_tangent,mc_Entity,invalidForVolume,m);
     if(mc_Entity.x>255)
       vMaterialIDs=mc_Entity.x-255.+39.;
     a=floor(a);
     a-=cameraPosition.xyz;
     int g=f();
     a=n(a,g);
     volumePosition=G(a,m,vFragDepth,invalidForVolume);
     if(mc_Entity.x==51||mc_Entity.x==50||mc_Entity.x==76)
       vFragDepth+=.9;
   }
   {
     v.xyz-=cameraPosition.xyz;
     v=shadowModelView*v;
     v=shadowProjection*v;
     gl_Position=v;
     float m=sqrt(gl_Position.x*gl_Position.x+gl_Position.y*gl_Position.y),a=1.f-SHADOW_MAP_BIAS+m*SHADOW_MAP_BIAS;
     gl_Position.xy*=.95f/a;
     gl_Position.xy*=.5;
     gl_Position.xy+=.5;
     if(z>.5)
       gl_Position.y-=1.;
     if(i>.5)
       gl_Position.x-=1.;
     gl_Position.z=mix(gl_Position.z,.5,.8);
     shadowPosition=gl_Position;
   }
 }
