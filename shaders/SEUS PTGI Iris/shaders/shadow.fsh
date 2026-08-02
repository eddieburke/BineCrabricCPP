#version 330 compatibility
#extension GL_ARB_shading_language_packing : enable
#extension GL_ARB_shader_bit_encoding : enable

#include "lib/Uniforms.inc"
#include "lib/Common.inc"
#include "lib/GBuffersCommon.inc"

in vec4 texcoord;
in vec4 color;
in vec4 viewPos;

in float materialIDs;
in float mcEntity;
in float isWater;

in float isVolumePass;
in float fragDepth;

in vec2 midTexCoord;

#include "lib/VoxelTracing.inc"

// Compatibility wrappers for deobfuscated call sites
int d() { return GetScreenVolumeSide(); }
int f() { return GetShadowVolumeSide(); }
vec3 n(vec2 v) { return ScreenUVToVolumeCoord(v); }
vec2 s(vec3 v) { return VolumeCoordToScreenUV(v); }
vec3 x(vec2 v) { return ShadowUVToVolumeCoord(v); }
vec2 d(vec3 v,int f) { return VolumeCoordToShadowUV(v, f); }
vec3 f(vec3 v,int y) { return WorldOffsetToVolumeCoordClamped(v, y); }
vec3 n(vec3 v,int y) { return WorldOffsetToVolumeCoord(v, y); }

 vec3 v(vec3 v) { return VolumeCoordToShadowWorldOffset(v); }
vec3 r(vec3 v) { return WorldOffsetToScreenVolumeCoord(v); }
vec3 p(vec3 v) { return ScreenVolumeCoordToWorldOffset(v); }
vec3 n() { return GetCameraVoxelDelta(); }
vec3 m(vec3 v) { return WorldPosToShadowMapCoord(v); }
vec3 d(vec3 v,vec3 f,vec2 n,vec2 r,vec4 x,vec4 i,inout float y,out vec2 z) { return AdjustVoxelHitForBlockShape(v, f, n, r, x, i, y, z); }
VoxelDDA i(Ray v) { return InitVoxelDDA(v); }
void e(inout VoxelDDA v) { StepVoxelDDA(v); }
void d(in Ray v,in vec3 f[2],out float i,out float x) { RayAABBIntersectDistances(v, f, i, x); }
bool e(const vec3 v,const vec3 f,Ray i,out vec2 r) { return RayAABBIntersect(v, f, i, r); }
bool d(const vec3 v,const vec3 f,Ray i,inout float x,inout vec3 y) { return RayAABBHitCloser(v, f, i, x, y); }
vec3 e(vec3 v,vec3 f,vec3 y,vec3 x,int z) { return SampleShadowedSunlightWithCaustics(v, f, y, x, z); }
vec3 f(vec3 f,vec3 x,vec3 y,vec3 z,int n) { return SampleShadowedSunlightStained(f, x, y, z, n); }
vec3 i(vec3 v,vec3 f,vec3 y,vec3 x,int z) { return SampleShadowedSunlightStained(v, f, y, x, z); }
vec4 w(GIBufferData v) { return PackGIBufferData(v); }
GIBufferData h(vec4 v) { return UnpackGIBufferData(v); }
GIBufferData R(vec2 v) { return SampleGIBuffer(v); }
float R(float v,float f) { return ReflectionStrengthFromSmoothness(v, f); }
bool R(vec3 v,float f,Ray x,bool y,inout float i,inout vec3 z) { return TraceBlockShape(v, f, x, y, i, z); }

 vec4 e(in sampler2D v,in vec2 f)
 {
   vec2 m=vec2(64.f,64.f);
   f*=m;
   f+=.5f;
   vec2 x=floor(f),i=fract(f);
   i.x=i.x*i.x*(3.f-2.f*i.x);
   i.y=i.y*i.y*(3.f-2.f*i.y);
   f=x+i;
   f-=.5f;
   f/=m;
   return texture2D(v,f);
 }
 float R(in float v,in float f,in float x)
 {
   if(v>f)
     return v;
   float n=2.f*x-f,y=2.f*f-3.f*x,c=v/f;
   return(n*c+y)*c*c+x;
 }
 float D(vec3 v)
 {
   float f=.5f;
   vec2 i=v.xz/20.f;
   i.xy-=v.y/20.f;
   i.x=-i.x;
   i.x+=FRAME_TIME/40.f*f;
   i.y-=FRAME_TIME/40.f*f;
   float r=1.f,m=r,x=0.f,n=e(noisetex,i*vec2(2.f,1.2f)+vec2(0.f,i.x*2.1f)).x;
   i/=2.1f;
   i.y-=FRAME_TIME/20.f*f;
   i.x-=FRAME_TIME/30.f*f;
   x+=n*.5;
   r=2.1f;
   m+=r;
   n=e(noisetex,i*vec2(2.f,1.4f)+vec2(0.f,-i.x*2.1f)).x;
   i/=1.5f;
   i.x+=FRAME_TIME/20.f*f;
   n*=r;
   x+=n;
   r=17.25f;
   m+=r;
   n=e(noisetex,i*vec2(1.f,.75f)+vec2(0.f,i.x*1.1f)).x;
   i/=1.5f;
   i.x-=FRAME_TIME/55.f*f;
   n*=r;
   x+=n;
   r=15.25f;
   m+=r;
   n=e(noisetex,i*vec2(1.f,.75f)+vec2(0.f,-i.x*1.7f)).x;
   i/=1.9f;
   i.x+=FRAME_TIME/155.f*f;
   n*=r;
   x+=n;
   r=29.25f;
   m+=r;
   n=abs(e(noisetex,i*vec2(1.f,.8f)+vec2(0.f,-i.x*1.7f)).x*2.f-1.f);
   i/=2.f;
   i.x+=FRAME_TIME/155.f*f;
   n=1.f-R(n,.2f,.1f);
   n*=r;
   x+=n;
   r=15.25f;
   m+=r;
   n=abs(e(noisetex,i*vec2(1.f,.8f)+vec2(0.f,i.x*1.7f)).x*2.f-1.f);
   n=1.f-R(n,.2f,.1f);
   n*=r;
   x+=n;
   x/=m;
   return x;
 }
 void main() {
	vec4 texSample = texture2D(texture, texcoord.xy, 0);
	vec3 albedo = texSample.xyz * color.xyz;
	float alpha = 1.0;

	if (isVolumePass < 0.5) {
		// Shadow-map write
		alpha = min(texSample.w * 7.0, 1.0);
		vec3 worldPos = (shadowModelViewInverse * vec4(viewPos.xyz, 1.0)).xyz;
		worldPos += cameraPosition.xyz;
		gl_FragData[0] = vec4(albedo.xyz, alpha);
		gl_FragData[1] = vec4(worldPos.y / 256.0, 1.0 - isWater, 0.0, alpha);
	} else {
		// Voxel-volume write (atlas cell chosen by gl_Position from volumePosition).
		// Invalid / oversized tris are already skipped in shadow.gsh.
		albedo *= albedo; // store linear-ish intensity
		if (abs(mcEntity - 50.0) < 0.1) {
			albedo.xyz = GetColorTorchlight() * 0.1 * GI_LIGHT_TORCH_INTENSITY;
		}
		if (abs(mcEntity - 76.0) < 0.1) {
			albedo.xyz = vec3(1.0, 0.02, 0.01) * 0.05 * GI_LIGHT_TORCH_INTENSITY;
		}
		if (abs(mcEntity - 51.0) < 0.1) {
			albedo.xyz = vec3(2.0, 0.35, 0.025);
		}
		float colorVariance = clamp((abs(color.x - color.y) + abs(color.x - color.z) + abs(color.y - color.z)) * 500.0, 0.0, 1.0);
		gl_FragData[0] = vec4(albedo.xyz, (materialIDs + 0.1) / 255.0 * alpha);
		gl_FragData[1] = vec4(midTexCoord.xy, colorVariance, dot(texSample.xyz, vec3(0.33333)));
	}
}
