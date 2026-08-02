#version 330 compatibility

#define OLD_LIGHTING_FIX		//In newest versions of the shaders mod/optifine, old lighting isn't removed properly. If OldLighting is On and this is enabled, you'll get proper results in any shaders mod/minecraft version.

#define GLOWING_REDSTONE_BLOCK // If enabled, redstone blocks are treated as light sources for GI
#define GLOWING_LAPIS_LAZULI_BLOCK // If enabled, lapis lazuli blocks are treated as light sources for GI

#define GENERAL_GRASS_FIX

#include "lib/Uniforms.inc"
#include "lib/Common.inc"

in vec4 mc_Entity;
in vec4 at_tangent;
in vec4 mc_midTexCoord;

out vec4 color;
out vec4 texcoord;
out vec4 lmcoord;
out vec3 worldPosition;
out vec3 viewPos;

out vec3 worldNormal;

out vec2 blockLight;

out float materialIDs;

#include "lib/Materials.inc"

#include "lib/VoxelTracing.inc"

// Compatibility wrappers for deobfuscated call sites
int f() { return GetScreenVolumeSide(); }
int s() { return GetShadowVolumeSide(); }
vec3 d(vec2 v) { return ScreenUVToVolumeCoord(v); }
vec2 n(vec3 v) { return VolumeCoordToScreenUV(v); }
vec3 x(vec2 v) { return ShadowUVToVolumeCoord(v); }
vec3 f(vec3 v,int y) { return WorldOffsetToVolumeCoordClamped(v, y); }
vec3 n(vec3 v,int y) { return WorldOffsetToVolumeCoord(v, y); }
vec3 d() { return GetCameraVoxelDelta(); }
vec3 m(vec3 v) { return WorldPosToShadowMapCoord(v); }

 vec3 d(vec3 v,vec3 m,vec2 x,vec2 y,vec4 n,vec4 r,inout float i,out vec2 f) { return AdjustVoxelHitForBlockShape(v, m, x, y, n, r, i, f); }
VoxelDDA e(Ray v) { return InitVoxelDDA(v); }
void i(inout VoxelDDA v) { StepVoxelDDA(v); }
bool e(const vec3 v,const vec3 x,Ray r,out vec2 i) { return RayAABBIntersect(v, x, r, i); }
bool d(const vec3 v,const vec3 x,Ray r,inout float y,inout vec3 z) { return RayAABBHitCloser(v, x, r, y, z); }
vec3 e(vec3 v,vec3 x,vec3 y,vec3 i,int z) { return SampleShadowedSunlightWithCaustics(v, x, y, i, z); }
vec3 f(vec3 x,vec3 f,vec3 y,vec3 i,int z) { return SampleShadowedSunlightStained(x, f, y, i, z); }
vec3 i(vec3 v,vec3 x,vec3 y,vec3 f,int z) { return SampleShadowedSunlightStained(v, x, y, f, z); }
vec4 w(GIBufferData v) { return PackGIBufferData(v); }
GIBufferData G(vec4 v) { return UnpackGIBufferData(v); }
GIBufferData h(vec2 v) { return SampleGIBuffer(v); }
float G(float v,float y) { return ReflectionStrengthFromSmoothness(v, y); }
bool G(vec3 v,float x,Ray y,bool m,inout float i,inout vec3 z) { return TraceBlockShape(v, x, y, m, i, z); }

 void main()
 {
   color=gl_Color;
   texcoord=gl_MultiTexCoord0;
   lmcoord=gl_TextureMatrix[1]*gl_MultiTexCoord1;
   blockLight.x=clamp(lmcoord.x*33.05f/32.f-.0328125f,0.f,1.f);
   blockLight.y=clamp(lmcoord.y*33.75f/32.f-.0328125f,0.f,1.f);
   worldNormal=gl_Normal;
   vec4 x=gbufferModelViewInverse*gl_ModelViewMatrix*gl_Vertex;
   worldPosition=x.xyz+cameraPosition.xyz;
   viewPos=(gl_ModelViewMatrix*gl_Vertex).xyz;
   materialIDs=MAT_ID_OPAQUE;
   float v=0.f,s=abs(normalize(gl_Normal.xz).x),i=abs(gl_Normal.y);
   if(mc_Entity.x==31.||mc_Entity.x==38.f||mc_Entity.x==37.f||mc_Entity.x==1925.f||mc_Entity.x==1920.f||mc_Entity.x==1921.f||mc_Entity.x==2.&&gl_Normal.y<.5&&s>.01&&s<.99&&i<.9)
     materialIDs=MAT_ID_GRASS,v=1.f;
   #ifdef GENERAL_GRASS_FIX
   if(abs(worldNormal.x)>.01&&abs(worldNormal.x)<.99||abs(worldNormal.y)>.01&&abs(worldNormal.y)<.99||abs(worldNormal.z)>.01&&abs(worldNormal.z)<.99)
     materialIDs=MAT_ID_GRASS;
   #endif
   if(mc_Entity.x==175.f)
     materialIDs=MAT_ID_GRASS;
   if(mc_Entity.x==59.)
     materialIDs=MAT_ID_GRASS,v=1.f;
   if(mc_Entity.x==18.||mc_Entity.x==161.f)
     {
       if(color.x>.999&&color.y>.999&&color.z>.999)
         ;
       else
          materialIDs=MAT_ID_LEAVES;
       if(abs(color.x-color.y)>.001||abs(color.x-color.z)>.001||abs(color.y-color.z)>.001)
         materialIDs=MAT_ID_LEAVES;
     }
   if(mc_Entity.x==50)
     materialIDs=MAT_ID_TORCH;
   if(mc_Entity.x==10||mc_Entity.x==11)
     materialIDs=MAT_ID_LAVA;
   if(mc_Entity.x==89||mc_Entity.x==124||mc_Entity.x==169||mc_Entity.x==91)
     materialIDs=MAT_ID_GLOWSTONE;
   #ifdef GLOWING_REDSTONE_BLOCK
   if(mc_Entity.x==152)
     materialIDs=MAT_ID_GLOWSTONE;
   #endif
   #ifdef GLOWING_LAPIS_LAZULI_BLOCK
   if(mc_Entity.x==22)
     materialIDs=MAT_ID_GLOWSTONE;
   #endif
   #ifdef GLOWING_EMERALD_BLOCK
   if(mc_Entity.x==133)
     materialIDs=MAT_ID_GLOWSTONE;
   #endif
   if(mc_Entity.x==51)
     materialIDs=MAT_ID_FIRE;
   if(mc_Entity.x==188||mc_Entity.x==189||mc_Entity.x==190||mc_Entity.x==191)
     materialIDs=MAT_ID_LIT_FURNACE;
   float r=1.;
   if(color.x==1.&&color.y==1.&&color.z==1.)
     r=0.;
   #ifdef OLD_LIGHTING_FIX
   if(v<.1&&r>.5)
     {
       if(worldNormal.x>.85)
         color.xyz*=1./.6;
       if(worldNormal.x<-.85)
         color.xyz*=1./.6;
       if(worldNormal.z>.85)
         color.xyz*=1.25;
       if(worldNormal.z<-.85)
         color.xyz*=1.25;
       if(worldNormal.y<-.85)
         color.xyz*=2.;
     }
   #endif
   gl_Position=gl_ProjectionMatrix*gbufferModelView*x;
   gl_Position.xyz/=gl_Position.w;
   TemporalJitterProjPos(gl_Position);
   gl_Position.xyz*=gl_Position.w;
 }
