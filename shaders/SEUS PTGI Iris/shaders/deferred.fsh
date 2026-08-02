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

/////////ADJUSTABLE VARIABLES//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////ADJUSTABLE VARIABLES//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//#define HALF_RES_TRACE

/////////INTERNAL VARIABLES////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////INTERNAL VARIABLES////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Do not change the name of these variables or their type. The Shaders Mod reads these lines and determines values to send to the inner-workings
//of the shaders mod. The shaders mod only reads these lines and doesn't actually know the real value assigned to these variables in GLSL.
//Some of these variables are critical for proper operation. Change at your own risk.

const bool colortex4Clear = false;
const bool colortex5Clear = false;
//END OF INTERNAL VARIABLES//

in vec4 texcoord;

in float timeMidnight;

in vec3 colorSunlight;
in vec3 colorSkylight;
in vec3 colorSkyUp;
in vec3 colorTorchlight;

in vec4 skySHR;
in vec4 skySHG;
in vec4 skySHB;

in vec3 worldLightVector;
in vec3 worldSunVector;

in mat4 gbufferPreviousModelViewInverse;
in mat4 gbufferPreviousProjectionInverse;

#include "lib/Uniforms.inc"
#include "lib/Common.inc"

#include "lib/Materials.inc"

/////////////////////////FUNCTIONS/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////FUNCTIONS/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// vec4 GetViewPosition(in vec2 coord, in float depth) 
// {	
// 	vec2 tcoord = coord;
// 	TemporalJitterProjPosInv01(tcoord);

// 	vec4 fragposition = gbufferProjectionInverse * vec4(tcoord.s * 2.0f - 1.0f, tcoord.t * 2.0f - 1.0f, 2.0f * depth - 1.0f, 1.0f);
// 		 fragposition /= fragposition.w;

	
// 	return fragposition;
// }

vec2 GetNearFragment(vec2 coord, float depth, out float minDepth)
{
	
	
	vec2 texel = 1.0 / vec2(viewWidth, viewHeight);
	vec4 depthSamples;
	depthSamples.x = texture2D(depthtex1, coord + texel * vec2(1.0, 1.0)).x;
	depthSamples.y = texture2D(depthtex1, coord + texel * vec2(1.0, -1.0)).x;
	depthSamples.z = texture2D(depthtex1, coord + texel * vec2(-1.0, 1.0)).x;
	depthSamples.w = texture2D(depthtex1, coord + texel * vec2(-1.0, -1.0)).x;

	vec2 targetFragment = vec2(0.0, 0.0);

	if (depthSamples.x < depth)
		targetFragment = vec2(1.0, 1.0);
	if (depthSamples.y < depth)
		targetFragment = vec2(1.0, -1.0);
	if (depthSamples.z < depth)
		targetFragment = vec2(-1.0, 1.0);
	if (depthSamples.w < depth)
		targetFragment = vec2(-1.0, -1.0);

	minDepth = min(min(min(depthSamples.x, depthSamples.y), depthSamples.z), depthSamples.w);

	return coord + texel * targetFragment;
}

#include "lib/GBufferData.inc"

#include "lib/VoxelTracing.inc"
#include "lib/GIHelpers.inc"

// Compatibility wrappers for deobfuscated call sites
int d() { return GetScreenVolumeSide(); }
int f() { return GetShadowVolumeSide(); }
vec3 v(vec2 v) { return ScreenUVToVolumeCoord(v); }
vec2 s(vec3 v) { return VolumeCoordToScreenUV(v); }
vec3 x(vec2 v) { return ShadowUVToVolumeCoord(v); }
vec2 d(vec3 v,int y) { return VolumeCoordToShadowUV(v, y); }
vec3 f(vec3 v,int y) { return WorldOffsetToVolumeCoordClamped(v, y); }
vec3 s(vec3 v,int y) { return WorldOffsetToVolumeCoord(v, y); }

 vec3 m(vec3 v) { return VolumeCoordToShadowWorldOffset(v); }
vec3 e(vec3 v) { return WorldOffsetToScreenVolumeCoord(v); }
vec3 n(vec3 v) { return ScreenVolumeCoordToWorldOffset(v); }
vec3 e() { return GetCameraVoxelDelta(); }
vec3 r(vec3 v) { return WorldPosToShadowMapCoord(v); }
vec3 d(vec3 v,vec3 f,vec2 n,vec2 z,vec4 i,vec4 y,inout float x,out vec2 r) { return AdjustVoxelHitForBlockShape(v, f, n, z, i, y, x, r); }
VoxelDDA p(Ray v) { return InitVoxelDDA(v); }
void i(inout VoxelDDA v) { StepVoxelDDA(v); }
void d(in Ray v,in vec3 f[2],out float i,out float y) { RayAABBIntersectDistances(v, f, i, y); }
vec3 d(const vec3 v,const vec3 f,vec3 y) { return AABBSurfaceNormal(v, f, y); }
bool e(const vec3 v,const vec3 f,Ray m,out vec2 i) { return RayAABBIntersect(v, f, m, i); }
bool d(const vec3 v,const vec3 f,Ray m,inout float y,inout vec3 x) { return RayAABBHitCloser(v, f, m, y, x); }
vec3 e(vec3 v,vec3 f,vec3 y,vec3 z,int x) { return SampleShadowedSunlightWithCaustics(v, f, y, z, x); }
vec3 f(vec3 v,vec3 f,vec3 y,vec3 z,int x) { return SampleShadowedSunlightVolume(v, f, y, z, x); }
vec3 i(vec3 v,vec3 f,vec3 y,vec3 z,int x) { return SampleShadowedSunlightStained(v, f, y, z, x); }
vec4 h(GIBufferData v) { return PackGIBufferData(v); }
GIBufferData w(vec4 v) { return UnpackGIBufferData(v); }
GIBufferData g(vec2 v) { return SampleGIBuffer(v); }
float e(float v,float y) { return ReflectionStrengthFromSmoothness(v, y); }
bool d(vec3 v,float y,Ray f,bool z,inout float x,inout vec3 i) { return TraceBlockShape(v, y, f, z, x, i); }

 vec2 c(inout float seed) { return Hash2(seed); }
 float G(vec2 uv) { return R2Noise(uv); }
 vec3 D(vec2 uv) { return SampleBlueNoise(uv); }
 vec3 D(vec3 normal, vec2 rand) { return SampleHemisphere(normal, rand); }
 vec3 y(inout float seed) { return RandomUnitVector(seed); }
 vec3 G(vec3 v,vec3 y)
 {
   vec2 f=s(e(m(v)+y+1.+e()));
   vec3 n=g(f).irradiance;
   return n;
 }
 vec3 D()
 {
   vec2 y=s(v(texcoord.xy)+e()/d());
   vec3 n=g(y).irradiance;
   return n;
 }
 // Path-traced diffuse GI for the current pixel (legacy overload name D).
vec3 D(vec3 v,vec3 m,vec3 y,vec3 n,vec3 x,MaterialMask z,float s,vec2 r,float t,out float c)
 {
   float w=fract(frameCounter*.0123456),e=1.;
   #ifdef SUNLIGHT_LEAK_FIX
   if(isEyeInWater<1)
     e=saturate(s*100.);
   #endif
   float A=1.;
   #ifdef CAVE_GI_LEAK_FIX
   if(isEyeInWater<1)
     A=saturate(s*10.);
   #endif
   vec3 a=D(texcoord.xy+vec2(0.,0.)).xyz,h=D(n,a.xy);
   c=10000.;
   vec3 M=h;
   #ifdef GI_SCREEN_SPACE_TRACING
   bool g=false;
   {
     const int Y=5;
     float T=.25*-m.z;
     T=mix(T,.8,.5);
     float l=.07*-m.z;
     l=mix(l,1.,.5);
     l=.6;
     vec2 o=texcoord.xy;
     vec3 R=m.xyz,b=normalize((gbufferModelView*vec4(h.xyz,0.)).xyz);
     for(int S=0;
S<Y;
S++)
       {
         float W=float(S),k=(W+.5+a.z)/float(Y),F=T*k;
         vec3 C=m.xyz+b*F,P=ProjectBack(C),U=GetViewPositionNoJitter(P.xy,GetDepth(P.xy)).xyz;
         float H=length(C)-length(U)-.02;
         if(H>0.&&H<l)
           {
             g=true;
             o=P.xy;
             R=U.xyz;
             break;
           }
       }
     vec3 S=(gbufferModelViewInverse*vec4(R,1.)).xyz;
     S+=Fract01(cameraPosition.xyz+.5)+.5;
     if(g)
       {
         vec3 C=pow(texture2DLod(colortex7,o.xy-r*.5,0).xyz,vec3(2.2));
         C*=1.-saturate(t*1.1);
         return C*100.;
       }
   }
   #endif
   int o=f();
   float C=1./float(o);
   vec3 l=v+y*(.0001*length(v));
   l+=Fract01(cameraPosition.xyz+.5);
   Ray T=MakeRay(f(l,o)*o-vec3(1.,1.,1.),h);
   VoxelDDA P=p(T);
   vec3 S=vec3(1.),R=vec3(0.);
   float W=0.,b=1.;
   {
     vec4 F=vec4(0.);
     vec3 Y=vec3(0.);
     float H=.5;
     for(int k=0;
k<DIFFUSE_TRACE_LENGTH;
k++)
       {
         Y=P.voxelPos/float(o);
         vec2 U=d(Y,o);
         F=texture2DLod(shadowcolor,U,0);
         W=F.w*255.;
         float J=1.-step(.5,abs(W-241.));
         vec3 I=F.xyz;
         R+=I*J*H*.5;
         #ifdef GI_LEAF_TRANSPARENCY
         if(abs(W-36.)<.1)
           {
             if(a.z<pow(.2,b))
               {
                 i(P);
                 H=1.;
                 b+=1.;
                 S*=pow(F.xyz,vec3(.25));
                 continue;
               }
           }
         #endif
         if(W<240.)
           {
             if(d(P.voxelPos,W,T,k==0,c,M))
               {
                 break;
               }
           }
         i(P);
         H=1.;
       }
     float k=0.;
     if(W<1.f||W>254.f)
       {
         vec3 U=T.direction;
         if(isEyeInWater>0)
           U=refract(U,vec3(0.,-1.,0.),1.3333);
         vec3 I=SkyShading(U,worldSunVector,rainStrength);
         I*=saturate(U.y*10.+1.);
         I=DoNightEyeAtNight(I*12.,timeMidnight)*.083333;
         I=TintUnderwaterDepth(I);
         if(length(U)<.1)
           k=300.;
         vec3 J=I*A*S;
         #ifdef CLOUDS_IN_GI
         CloudPlane(J,-T.direction,worldLightVector,colorSunlight,colorSkyUp,J,timeMidnight,false);
         #endif
         R+=J*.1;
       }
     else
       {
         if(abs(W-31.)<.1)
           R+=.09*S*F.xyz*GI_LIGHT_BLOCK_INTENSITY;
         if(W>=32.&&W<=35.)
           {
             float I=0.;
             if(abs(W-32.)<.1)
               I=max(-M.z,0.);
             if(abs(W-33.)<.1)
               I=max(M.x,0.);
             if(abs(W-34.)<.1)
               I=max(M.z,0.);
             if(abs(W-35.)<.1)
               I=max(-M.x,0.);
             R+=.04*S*I*vec3(2.,.35,.025)*GI_LIGHT_BLOCK_INTENSITY;
           }
         if(W<240.)
           {
             vec3 I=saturate(F.xyz);
             S*=I;
             R+=G(Y,M)*S*1.5;
             const float J=2.4;
             vec3 U=i(l+T.direction*c-1.,worldLightVector,M,h,o),u=DoNightEyeAtNight(U*S*J*colorSunlight*e*A*12.,timeMidnight)/12.;
             R+=u;
             k=c;
           }
       }
     UnderwaterFog(R,k,h,colorSkyUp,colorSunlight);
   }
   if(z.grass<.5)
     R/=saturate(dot(n,h))+.01,R*=saturate(dot(y,h));
   return R;
 }
 vec3 G()
 {
   int y=f();
   vec3 x=v(texcoord.xy),m=n(x),c=f(m-vec3(1.,1.,0.),y);
   vec2 r=d(c,y);
   float s=1.;
   #ifdef CAVE_GI_LEAK_FIX
   if(isEyeInWater<1)
     s*=saturate(eyeBrightnessSmooth.y/240.*20.);
   #endif
   float z=1000.;
   z=min(z,texture2DLod(shadowcolor,d(f(m-vec3(0.,0.,0.),y),y)+vec2(.5,.5)/float(4096),0).w);
   z=min(z,texture2DLod(shadowcolor,d(f(m-vec3(0.,0.,0.),y),y)+vec2(-.5,-.5)/float(4096),0).w);
   z=min(z,texture2DLod(shadowcolor,d(f(m-vec3(0.,1.,0.),y),y)+vec2(0.,0.)/float(4096),0).w);
   z=min(z,texture2DLod(shadowcolor,d(f(m-vec3(0.,-1.,0.),y),y)+vec2(0.,0.)/float(4096),0).w);
   float t=texture2DLod(shadowcolor,d(f(m-vec3(0.,0.,0.),y),y),0).w;
   if(z*255.>240.||t*255.<240.)
     return vec3(0.);
   vec3 w=vec3(0.);
   for(int e=0;
e<GI_SECONDARY_SAMPLES;
e++)
     {
       float a=sin(frameTimeCounter*1.1)+m.x*.11+m.y*.12+m.z*.13+e*.1;
       vec3 h=normalize(rand(vec2(a))*2.-1.);
       h.x+=h.x==h.y||h.x==h.z?.01:0.;
       h.y+=h.y==h.z?.01:0.;
       vec3 W=m+vec3(1.,1.,1.);
       Ray o=MakeRay(f(W,y)*y-vec3(1.,1.,1.),h);
       VoxelDDA M=p(o);
       vec3 S=vec3(1.);
       float I=1000.;
       for(int R=0;
R<1;
R++)
         {
           vec4 g=vec4(0.);
           float l=0.;
           vec3 U=vec3(0.);
           float C=.2;
           for(int k=0;
k<DIFFUSE_TRACE_LENGTH;
k++)
             {
               U=M.voxelPos/float(y);
               vec2 J=d(U,y);
               g=texture2DLod(shadowcolor,J,0);
               l=g.w*255.;
               float A=1.-step(.5,abs(l-241.));
               vec3 T=g.xyz;
               w+=T*C*A*(k==0?.5:1.);
               if(l<240.)
                 {
                   break;
                 }
               i(M);
               C=saturate(C*1.3);
             }
           I=distance(M.voxelPos.xyz,M.voxelOrigin.xyz);
           float k=0.,A=1.;
           if(abs(M.voxelPos.x-M.voxelOrigin.x)<2||abs(M.voxelPos.y-M.voxelOrigin.y)<2||abs(M.voxelPos.z-M.voxelOrigin.z)<2)
             A=0.;
           if(l<1.f||l>254.f)
             {
               vec3 T=o.direction;
               if(isEyeInWater>0)
                 T=refract(T,vec3(0.,-1.,0.),1.3333);
               vec3 F=SkyShading(T,worldSunVector,rainStrength);
               F*=saturate(T.y*10.+1.);
               F=DoNightEyeAtNight(F*12.,timeMidnight)*.083333;
               F=TintUnderwaterDepth(F);
               if(isEyeInWater>0)
                 ;
               if(length(T)<.1)
                 k=300.;
               vec3 J=F*s*S;
               CloudPlane(J,-o.direction,worldLightVector,colorSunlight,colorSkyUp,J,timeMidnight,false);
               w+=J*.1;
             }
           vec3 T=-M.stepMask*M.stepDir;
           if(abs(l-31.)<.1)
             w+=.09*g.xyz*GI_LIGHT_BLOCK_INTENSITY;
           if(l>=32.&&l<=35.)
             {
               float J=0.;
               if(abs(l-32.)<.1)
                 J=max(-T.z,0.);
               if(abs(l-33.)<.1)
                 J=max(T.x,0.);
               if(abs(l-34.)<.1)
                 J=max(T.z,0.);
               if(abs(l-35.)<.1)
                 J=max(-T.x,0.);
               w+=.02*S*J*vec3(2.,.35,.025)*GI_LIGHT_BLOCK_INTENSITY;
             }
           if(l<240.)
             {
               vec3 J=saturate(g.xyz);
               S*=J;
               vec3 F=-(M.stepMask*M.stepDir);
               const float H=2.4;
               vec3 Y=f(U,worldLightVector,F,h,y),P=DoNightEyeAtNight(Y*H*colorSunlight*A*S*s*12.,timeMidnight)/12.;
               w+=P*2.;
               w+=G(U,F)*S;
               k=I;
             }
           {
             vec2 J=IntersectSphere(m,o.direction,vec3(0.,1.5,0.),.75);
             if(I>J.y&&J.y>-.5)
               ;
           }
           UnderwaterFog(w,k,h,colorSkyUp,colorSunlight);
         }
     }
   w/=float(GI_SECONDARY_SAMPLES);
   return saturate(w);
 }
 vec4 T(float v)
 {
   float y=v*v,m=y*v;
   vec4 f;
   f.x=-m+3*y-3*v+1;
   f.y=3*m-6*y+4;
   f.z=-3*m+3*y+3*v+1;
   f.w=m;
   return f/6.f;
 }
 vec4 T(in sampler2D v,in vec2 y)
 {
   vec2 f=vec2(viewWidth,viewHeight);
   y*=f;
   y-=.5;
   float m=fract(y.x),i=fract(y.y);
   y.x-=m;
   y.y-=i;
   vec4 n=T(m),c=T(i),x=vec4(y.x-.5,y.x+1.5,y.y-.5,y.y+1.5),s=vec4(n.x+n.y,n.z+n.w,c.x+c.y,c.z+c.w),r=x+vec4(n.y,n.w,c.y,c.w)/s,z=texture2DLod(v,vec2(r.x,r.z)/f,0),t=texture2DLod(v,vec2(r.y,r.z)/f,0),w=texture2DLod(v,vec2(r.x,r.w)/f,0),h=texture2DLod(v,vec2(r.y,r.w)/f,0);
   float o=s.x/(s.x+s.y),k=s.z/(s.z+s.w);
   return mix(mix(h,w,o),mix(t,z,o),k);
 }
 bool c(vec3 v,vec3 y)
 {
   vec3 n=normalize(cross(dFdx(v),dFdy(v))),i=normalize(y-v),c=normalize(i);
   float f=.02+length(v)*.04;
   return distance(v,y)<f;
 }
 vec3 F(vec2 v)
 {
   vec2 y=vec2(viewWidth,viewHeight),m=1./y,f=v*y,x=floor(f-.5)+.5,n=f-x,z=n*n,c=n*z;
   float s=.5;
   vec2 i=-s*c+2.*s*z-s*n,r=(2.-s)*c-(3.-s)*z+1.,t=-(2.-s)*c+(3.-2.*s)*z+s*n,M=s*c-s*z,w=r+t,a=m*(x+t/w);
   vec3 l=texture2DLod(colortex4,vec2(a.x,a.y),0).xyz;
   vec2 o=m*(x-1.),h=m*(x+2.);
   vec4 J=vec4(texture2DLod(colortex4,vec2(a.x,o.y),0).xyz,1.)*(w.x*i.y)+vec4(texture2DLod(colortex4,vec2(o.x,a.y),0).xyz,1.)*(i.x*w.y)+vec4(l,1.)*(w.x*w.y)+vec4(texture2DLod(colortex4,vec2(h.x,a.y),0).xyz,1.)*(M.x*w.y)+vec4(texture2DLod(colortex4,vec2(a.x,h.y),0).xyz,1.)*(w.x*M.y);
   return max(vec3(0.),J.xyz*(1./J.w));
 }
 vec2 D(float v,vec2 f,out float y,out vec3 n,out vec4 i)
 {
   float x;
   vec2 c=GetNearFragment(texcoord.xy,v,x);
   y=texture2D(depthtex1,c).x;
   vec4 m=vec4(texcoord.xy*2.-1.,y*2.-1.,1.),r=gbufferProjectionInverse*m;
   r.xyz/=r.w;
   vec4 s=gbufferModelViewInverse*vec4(r.xyz,1.);
   i=s;
   i.xyz+=cameraPosition-previousCameraPosition;
   vec4 t=gbufferPreviousModelView*vec4(i.xyz,1.),w=gbufferPreviousProjection*vec4(t.xyz,1.);
   w.xyz/=w.w;
   n=m.xyz-w.xyz;
   float z=length(n.xy)*10.,o=clamp(z*500.,0.,1.);
   vec2 J=f.xy-n.xy*.5;
   if(y<.7)
     J=texcoord.xy;
   return J;
 }
 void main()
 {
   GBufferData v=GetGBufferData();
   MaterialMask y=CalculateMasks(v.materialID);
   vec4 f=GetViewPosition(texcoord.xy,v.depth),n=gbufferModelViewInverse*vec4(f.xyz,1.),i=gbufferModelViewInverse*vec4(f.xyz,0.);
   vec3 x=normalize(f.xyz),m=normalize(i.xyz),r=normalize((gbufferModelViewInverse*vec4(v.normal,0.)).xyz),z=normalize((gbufferModelViewInverse*vec4(v.geoNormal,0.)).xyz);
   float s=length(f.xyz),t=dot(v.mcLightmap.xy,vec2(.5));
   if(y.grass>.5)
     r=vec3(0.,1.,0.),z=vec3(0.,1.,0.);
   vec4 w=vec4(texcoord.xy,0.,0.);
   float a;
   vec3 M;
   vec4 o;
   vec2 T=D(v.depth,w.xy,a,M,o),l=T.xy;
   l-=(vec2(mod(frameCounter/2,2.f),mod(frameCounter,2.f))-.5)/vec2(viewWidth,viewHeight)*1.5;
   vec3 J=F(l.xy);
   GIBufferData k=g(T.xy);
   float W=1./(saturate(-dot(v.geoNormal,x))*100.+1.);
   vec4 U=vec4(T.xy,0.,0.);
   TemporalJitterProjPosPrevInv(U);
   vec4 I=gbufferPreviousProjectionInverse*vec4(T.xy*2.-1.,texture2DLod(colortex7,U.xy,0).w*2.-1.,1.);
   I/=I.w;
   vec3 R=(gbufferPreviousModelViewInverse*vec4(I.xyz,1.)).xyz;
   k.depth+=1.;
   k.depth=min(k.depth,2.);
   vec2 e=1./vec2(viewWidth,viewHeight),d=1.-e;
   float C=0.,A=1.-exp2(-k.depth);
   if(!c(o.xyz,R.xyz)||(T.x<e.x||T.x>d.x||T.y<e.y||T.y>d.y)||abs(W-k.auxR)>.01)
     A=0.,C=.99,k.depth=0.;
   float S;
   vec3 Y=D(n.xyz,f.xyz,r.xyz,z,m.xyz,y,v.mcLightmap.y,M.xy,C,S);
   Y=max(vec3(0.),Y);
   Y=mix(Y,J,vec3(A));
   C=max(C,mix(C,.9,saturate(-M.z*120.)));
   k.auxR=W;
   k.auxG=mix(k.auxG,C,mix(.5,1.,C));
   k.auxB=t;
   vec3 H=G();
   k.irradiance=mix(D(),H,vec3(.015));
   vec4 P=h(k);
   gl_FragData[0]=vec4(Y,saturate(a));
   gl_FragData[1]=vec4(P);
   gl_FragData[2]=vec4(Y,1.);
 }

/* RENDERTARGETS: 4,5,6 */
