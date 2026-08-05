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

const bool colortex6MipmapEnabled = false;

in vec4 texcoord;

in vec3 lightVector;
in vec3 worldLightVector;
in vec3 worldSunVector;

in float timeMidnight;

in vec3 colorSunlight;
in vec3 colorSkylight;
in vec3 colorSkyUp;
in vec3 colorTorchlight;

in vec4 skySHR;
in vec4 skySHG;
in vec4 skySHB;

#include "lib/GBufferData.inc"

// vec4 GetViewPosition(in vec2 coord, in float depth) 
// {	
// 	vec4 tcoord = vec4(coord.xy, 0.0, 0.0);

// 	vec4 fragposition = gbufferProjectionInverse * vec4(tcoord.s * 2.0f - 1.0f, tcoord.t * 2.0f - 1.0f, 2.0f * depth - 1.0f, 1.0f);
// 		 fragposition /= fragposition.w;

	
// 	return fragposition;
// }

#include "lib/Materials.inc"

#include "lib/VoxelTracing.inc"
#include "lib/GIHelpers.inc"

// Compatibility wrappers for deobfuscated call sites
int d() { return GetScreenVolumeSide(); }
int f() { return GetShadowVolumeSide(); }
vec3 e(vec2 v) { return ScreenUVToVolumeCoord(v); }
vec2 x(vec3 v) { return VolumeCoordToScreenUV(v); }
vec3 n(vec2 v) { return ShadowUVToVolumeCoord(v); }
vec3 e() { return GetCameraVoxelDelta(); }
vec3 m(vec3 v) { return WorldPosToShadowMapCoord(v); }
VoxelDDA w(Ray v) { return InitVoxelDDA(v); }
void p(inout VoxelDDA v) { StepVoxelDDA(v); }
void d(in Ray v,in vec3 f[2],out float i,out float z) { RayAABBIntersectDistances(v, f, i, z); }

 vec3 d(const vec3 v,const vec3 f,vec3 z)
 {
   const float x=1e-05;
   vec3 y=(f+v)*.5,m=(f-v)*.5,s=z-y,i=vec3(0.);
   i+=vec3(sign(s.x),0.,0.)*step(abs(abs(s.x)-m.x),x);
   i+=vec3(0.,sign(s.y),0.)*step(abs(abs(s.y)-m.y),x);
   i+=vec3(0.,0.,sign(s.z))*step(abs(abs(s.z)-m.z),x);
   return normalize(i);
 }
 bool e(const vec3 v,const vec3 f,Ray m,out vec2 i) { return RayAABBIntersect(v, f, m, i); }
bool d(const vec3 v,const vec3 f,Ray m,inout float x,inout vec3 z) { return RayAABBHitCloser(v, f, m, x, z); }
vec3 e(vec3 v,vec3 f,vec3 z,vec3 x,int y) { return SampleShadowedSunlightWithCaustics(v, f, z, x, y); }
vec3 f(vec3 z,vec3 f,vec3 y,vec3 x,int i) { return SampleShadowedSunlightStained(z, f, y, x, i); }
vec3 m(vec3 v,vec3 f,vec3 z,vec3 x,int y) { return SampleShadowedSunlightStained(v, f, z, x, y); }
vec4 h(GIBufferData v) { return PackGIBufferData(v); }
GIBufferData i(vec4 v) { return UnpackGIBufferData(v); }
GIBufferData G(vec2 v) { return SampleGIBuffer(v); }
float G(float v,float z) { return ReflectionStrengthFromSmoothness(v, z); }
bool G(vec3 v,float z,Ray f,bool y,inout float x,inout vec3 i) { return TraceBlockShape(v, z, f, y, x, i); }

 float h(float v,float z)
 {
   return 1./(v*(1.-z)+z);
 }
 void G(inout vec3 v,in vec3 z,in vec3 x,vec3 f,float y)
 {
   float r=length(z);
   r*=pow(eyeBrightnessSmooth.y/240.f,6.f);
   r*=rainStrength;
   float s=pow(exp(-r*3e-06),4.);
   vec3 c=vec3(dot(colorSkyUp,vec3(1.)));
   v=mix(c,v,vec3(s));
 }
 vec4 c(vec2 v)
 {
   vec2 z=vec2(v.x,(v.y-floor(mod(FRAME_TIME*60.f,60.f)))/60.f);
   return texture2DLod(colortex4,z.xy,0);
 }
 float c(vec3 v,float z)
 {
   vec3 f=v.xyz+cameraPosition.xyz,m=refract(worldLightVector,vec3(0.,1.,0.),.750188);
   f+=m*((v.y+cameraPosition.y)/m.y);
   vec4 s=c(mod(f.xz/4.,vec2(1.)))*13.;
   float x=pow(z/2.,.5),i=pow(s.x,saturate(x*.5+.5));
   i=mix(i,s.y,saturate(x-1.));
   i=mix(i,s.z,saturate(x-2.));
   i=mix(i,s.w,saturate(x-3.));
   return i;
 }
 float i(float v,float z)
 {
   return exp(-pow(v/(.9*z),2.));
 }
 float m(vec3 v,vec3 z)
 {
   return dot(abs(v-z),vec3(.3333));
 }
 vec3 R(vec2 uv) { return SampleBlueNoise(uv); }
 vec3 G(float v,float f,float m,vec3 z)
 {
   vec3 i;
   i.x=m*cos(v);
   i.y=m*sin(v);
   i.z=f;
   vec3 s=abs(z.y)<.999?vec3(0,0,1):vec3(1,0,0),x=normalize(cross(z,vec3(0.,1.,1.))),c=cross(x,z);
   return x*i.x+c*i.y+z*i.z;
 }
 vec3 G(vec2 v,float z,vec3 i)
 {
   float s=2*3.14159*v.x,x=sqrt((1-v.y)/(1+(z*z-1)*v.y)),f=sqrt(1-x*x);
   return G(s,x,f,i);
 }
 float a(float v)
 {
   return 2./(v*v+1e-07)-2.;
 }
 vec3 R(in vec2 v,in float z,in vec3 i)
 {
   float s=a(z),f=2*3.14159*v.x,x=pow(v.y,1.f/(s+1.f)),c=sqrt(1-x*x);
   return G(f,x,c,i);
 }
 float l(vec2 v)
 {
   return texture2DLod(colortex3,v,0).w;
 }
 float R(float v,float z)
 {
   return v/(z*20.01+1.);
 }
 vec2 a(vec2 v,float z)
 {
   vec2 s=v;
   mat2 m=mat2(cos(z),-sin(z),sin(z),cos(z));
   v=m*v;
   return v;
 }
 vec4 G(sampler2D v,float f,bool z,float s,float x,float i,float y)
 {
   GBufferData c=GetGBufferData();
   GBufferDataTransparent n=GetGBufferDataTransparent();
   bool r=n.depth<c.depth;
   if(r)
     c.normal=n.normal,c.smoothness=n.smoothness,c.metalness=0.,c.mcLightmap=n.mcLightmap,c.depth=n.depth;
   vec4 d=GetViewPosition(texcoord.xy,c.depth),w=gbufferModelViewInverse*vec4(d.xyz,1.),h=gbufferModelViewInverse*vec4(d.xyz,0.);
   vec3 t=normalize(d.xyz),o=normalize(h.xyz),p=normalize((gbufferModelViewInverse*vec4(c.normal,0.)).xyz);
   float e=GetDepthLinear(texcoord.xy),A=dot(-t,c.normal.xyz),g=1.-c.smoothness,M=g*g,b=G(c.smoothness,c.metalness);
   vec4 W=texture2DLod(colortex6,texcoord.xy,0);
   float Y=Luminance(W.xyz);
   if(b<.001)
     return W;
   float T=f*.9;
   T*=min(M*20.,1.1);
   T*=mix(W.w,1.,1.);
   vec2 D=vec2(0.);
   if(z)
     {
       vec2 P=BlueNoiseTemporal(texcoord.xy).xy*.99+.005;
       D=P-.5;
     }
   float P=0.,S=1.1,u=R(s,c.totalTexGrad)/(M+.0001),L=R(x,c.totalTexGrad);
   vec4 U=vec4(0.),I=vec4(0.);
   float F=0.;
   vec4 H=vec4(vec3(i),1.);
   H.xyz=vec3(.5);
   H.xyz*=W.w*.95+.05;
   float J=c.smoothness;
   vec2 E=normalize(cross(c.normal,t).xy),k=a(E,1.5708);
   float K=1.-pow(1.-saturate(A),1.);
   E*=mix(.1675,.5,K);
   k*=mix(mix(.7,.7,M),.5,K);
   vec3 B=reflect(-t,c.normal);
   int N=0;
   for(int V=-1;
V<=1;
V++)
     {
       for(int Q=-1;
Q<=1;
Q++)
         {
           vec2 C=vec2(V,Q)+D;
           C=C.x*E+C.y*k;
           C*=T*1.5/vec2(viewWidth,viewHeight);
           vec2 O=texcoord.xy+C.xy;
           float q=length(C*vec2(viewWidth,viewHeight));
           if(q*.025>W.w+.1)
             {
               continue;
             }
           O=clamp(O,4./vec2(viewWidth,viewHeight),1.-4./vec2(viewWidth,viewHeight));
           vec4 X=texture2DLod(colortex6,O,0);
           vec3 j=GetNormals(O);
           float Z=GetDepthLinear(O),ab=pow(saturate(dot(B,reflect(-t,j))),105./M),ac=exp(-(abs(Z-e)*S)),ad=exp(-(m(X.xyz,W.xyz)*P)),ae=exp(-abs(J-l(O))*L),af=ab*ac*ad*ae;
           U+=vec4(pow(length(X.xyz),H.x)*normalize(X.xyz+1e-05),X.w)*af;
           F+=af;
           I+=X;
           N++;
         }
     }
   U/=F+.0001;
   U.xyz=pow(length(U.xyz),1./H.x)*normalize(U.xyz+1e-06);
   vec4 O=U;
   if(F<.001)
     O=W;
   return O;
 }
 void main()
 {
   GBufferData v=GetGBufferData();
   GBufferDataTransparent z=GetGBufferDataTransparent();
   MaterialMask s=CalculateMasks(v.materialID),f=CalculateMasks(z.materialID);
   bool i=z.depth<v.depth;
   if(i)
     v.normal=z.normal,v.smoothness=z.smoothness,v.metalness=0.,v.mcLightmap=z.mcLightmap,v.depth=z.depth,f.sky=0.;
   vec4 c=GetViewPosition(texcoord.xy,v.depth),x=gbufferModelViewInverse*vec4(c.xyz,1.),m=gbufferModelViewInverse*vec4(c.xyz,0.);
   vec3 n=normalize(c.xyz),r=normalize(m.xyz),y=normalize((gbufferModelViewInverse*vec4(v.normal,0.)).xyz);
   float t=ExpToLinearDepth(v.depth),a=1.-v.smoothness,w=a*a,o=G(v.smoothness,v.metalness);
   int O=0;
   vec4 p=texture2DLod(colortex6,texcoord.xy,O),e=p;
   float W=1.-v.smoothness,M=W*W;
   vec3 d=y,l=-r,A=normalize(reflect(-l,d)+d*M),K=normalize(l+A);
   float g=saturate(dot(d,A)),b=saturate(dot(d,l)),R=saturate(dot(d,K)),S=saturate(dot(A,K)),P=v.metalness*.98+.02,L=pow(1.-S,5.),T=P+(1.-P)*L,D=M/2.,k=h(g,D)*h(b+.8,D),U=g*T*k;
   e.xyz*=mix(vec3(1.),v.albedo.xyz,vec3(v.metalness));
   U=mix(U,1.,v.metalness);
   if(v.depth>.99999)
     U=0.;
   if(f.water>.5&&isEyeInWater>0)
     {
       if(length(refract(l,d,1.3333))<.5)
         U=1.;
       else
          U=0.;
     }
   U*=G(v.smoothness,v.metalness);
   vec4 X=texture2DLod(colortex3,texcoord.xy,0);
   vec3 C=pow(X.xyz,vec3(2.2)),H=C;
   H*=120.;
   UnderwaterFog(H,length(m.xyz),r,colorSkyUp,colorSunlight);
   H=mix(H,e.xyz*12.,vec3(saturate(U)));
   H+=C*v.metalness*120.;
   if(f.sky<.5&&!i&&isEyeInWater<1)
     LandAtmosphericScattering(H,c.xyz,n.xyz,r.xyz,worldSunVector.xyz,1.);
   G(H,c.xyz,n.xyz,r.xyz,1.);
   {
     #ifdef GODRAYS
     #else
     if(isEyeInWater>0)
       #endif
     {
       float Y=BlueNoiseTemporal(texcoord.xy).x,E=120.;
       if(isEyeInWater>0)
         E=20.;
       vec3 F=vec3(0.),Q=(gbufferModelViewInverse*vec4(0.,0.,0.,1.)).xyz;
       for(int V=0;
V<10;
V++)
         {
           float N=float(V+Y)/float(10);
           vec3 J=r.xyz*E*N+Q;
           if(length(m.xyz)<length(J-Q))
             {
               break;
             }
           float B,j;
           vec3 q=WorldPosToShadowProjPos(J.xyz,B,j),I=shadow2DLod(shadowtex0,vec3(q.xy,q.z+1e-06),3).xxx;
           #ifdef GODRAYS_STAINED_GLASS_TINT
           float Z=shadow2DLod(shadowtex0,vec3(q.xy-vec2(.5,0.),q.z-1e-06),3).x;
           vec3 u=texture2DLod(shadowcolor,vec2(q.xy-vec2(.5,0.)),3).xyz;
           u*=u;
           I=mix(I,I*u,vec3(1.-Z));
           #endif
           if(isEyeInWater>0)
             {
               float ag=abs(texture2DLod(shadowcolor1,q.xy-vec2(0.,.5),4).x*256.-(J.y+cameraPosition.y)),ah=GetCausticsComposite(J,worldLightVector,ag),ai=shadow2DLod(shadowtex0,vec3(q.xy-vec2(0.,.5),q.z+1e-06),4).x;
               I=mix(I,I*ah,vec3(1.-ai));
               F+=I*exp(-GetWaterAbsorption()*(E*N))*exp(-GetWaterAbsorption()*ag);
             }
           else
              F+=I*colorSunlight*.2;
         }
       float I=dot(worldLightVector,r.xyz),q=1.;
       if(isEyeInWater>0)
         I=dot(refract(worldLightVector,vec3(0.,-1.,0.),.750019),r.xyz);
       else
          q=.5/(max(0.,pow(worldLightVector.y,2.)*2.)+.4);
       float j=I*I,u=PhaseMie(.8,I,j);
       H+=TintUnderwaterDepth(F*colorSunlight*GetWaterFogColor()*.075*u*q);
     }
   }
   H/=120.;
   H*=exp(-t*blindness);
   H=pow(H.xyz,vec3(.454545));
   gl_FragData[0]=vec4(H,X.w);
 }

/* RENDERTARGETS: 3 */
