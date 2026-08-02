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

in vec3 colorSunlight;
in vec3 colorSkylight;
in vec3 colorTorchlight;
in vec3 colorSkyUp;

in vec4 skySHR;
in vec4 skySHG;
in vec4 skySHB;

in vec3 worldLightVector;
in vec3 worldSunVector;

in float timeMidnight;

#include "lib/Uniforms.inc"
#include "lib/Common.inc"

/////////////////////////FUNCTIONS/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////FUNCTIONS/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

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

#include "lib/Materials.inc"
#include "lib/GBufferData.inc"

#include "lib/VoxelTracing.inc"
#include "lib/GIHelpers.inc"

// Compatibility wrappers for deobfuscated call sites
int e() { return GetScreenVolumeSide(); }
int f() { return GetShadowVolumeSide(); }
vec3 v(vec2 v) { return ScreenUVToVolumeCoord(v); }
vec2 s(vec3 v) { return VolumeCoordToScreenUV(v); }
vec3 d(vec2 v) { return ShadowUVToVolumeCoord(v); }
vec3 e(vec3 v,int y) { return WorldOffsetToVolumeCoordClamped(v, y); }
vec3 f(vec3 v,int y) { return WorldOffsetToVolumeCoord(v, y); }
vec2 d(vec3 v,int side) { return VolumeCoordToShadowUV(v, side); }
vec3 n(vec3 v) { return ScreenVolumeCoordToWorldOffset(v); }
vec3 x(vec3 v) { return WorldOffsetToScreenVolumeCoord(v); }

 vec3 r(vec3 v) { return ScreenVolumeCoordToWorldOffset(v); }
vec3 d() { return GetCameraVoxelDelta(); }
vec3 m(vec3 v) { return WorldPosToShadowMapCoord(v); }
vec3 d(vec3 v,vec3 f,vec2 i,vec2 y,vec4 d,vec4 s,inout float x,out vec2 r) { return AdjustVoxelHitForBlockShape(v, f, i, y, d, s, x, r); }
VoxelDDA p(Ray v) { return InitVoxelDDA(v); }
void i(inout VoxelDDA v) { StepVoxelDDA(v); }
void d(in Ray v,in vec3 f[2],out float i,out float y) { RayAABBIntersectDistances(v, f, i, y); }
bool e(const vec3 v,const vec3 f,Ray s,out vec2 i) { return RayAABBIntersect(v, f, s, i); }
bool d(const vec3 v,const vec3 f,Ray s,inout float y,inout vec3 x) { return RayAABBHitCloser(v, f, s, y, x); }
vec3 e(vec3 v,vec3 f,vec3 y,vec3 z,int x) { return SampleShadowedSunlightWithCaustics(v, f, y, z, x); }
vec3 f(vec3 v,vec3 f,vec3 y,vec3 z,int x) { return SampleShadowedSunlightStained(v, f, y, z, x); }
vec3 i(vec3 v,vec3 f,vec3 y,vec3 z,int x) { return SampleShadowedSunlightStained(v, f, y, z, x); }
vec4 h(GIBufferData v) { return PackGIBufferData(v); }
GIBufferData w(vec4 v) { return UnpackGIBufferData(v); }
GIBufferData G(vec2 v) { return SampleGIBuffer(v); }
float G(float v,float y) { return ReflectionStrengthFromSmoothness(v, y); }
bool G(vec3 v,float y,Ray f,bool z,inout float i,inout vec3 t) { return TraceBlockShape(v, y, f, z, i, t); }

 vec2 g(inout float seed) { return Hash2(seed); }
 vec3 c(vec2 uv) { return SampleBlueNoise(uv); }
 vec3 G(vec3 v,inout float y,int z)
 {
   vec2 f=c(texcoord.xy+vec2(y+=.1,y+=.1)).xy;
   f=fract(f+g(y)*.1);
   float x=6.28319*f.x,i=sqrt(f.y);
   vec3 t=normalize(cross(v,vec3(0.,1.,1.))),r=cross(v,t),m=t*cos(x)*i+r*sin(x)*i+v.xyz*sqrt(1.-f.y);
   return m;
 }
 vec3 c(vec3 v,vec3 y)
 {
   vec2 f=s(x(n(v)+y+1.));
   vec3 z=G(f).irradiance;
   return z;
 }
 vec3 G()
 {
   vec2 y=s(v(texcoord.xy)+d()/e());
   vec3 x=G(y).irradiance;
   return x;
 }
 vec3 G(float v,float f,float x,vec3 y)
 {
   vec3 r;
   r.x=x*cos(v);
   r.y=x*sin(v);
   r.z=f;
   vec3 z=abs(y.y)<.999?vec3(0,0,1):vec3(1,0,0),i=normalize(cross(y,vec3(0.,1.,1.))),t=cross(i,y);
   return i*r.x+t*r.y+y*r.z;
 }
 vec3 c(vec2 v,float y,vec3 t)
 {
   float x=2*3.14159*v.x,z=sqrt((1-v.y)/(1+(y*y-1)*v.y)),i=sqrt(1-z*z);
   return G(x,z,i,t);
 }
 float l(float v)
 {
   return 2./(v*v+1e-07)-2.;
 }
 vec3 e(in vec2 v,in float y,in vec3 t)
 {
   float x=l(y),f=2*3.14159*v.x,z=pow(v.y,1.f/(x+1.f)),i=sqrt(1-z*z);
   return G(f,z,i,t);
 }
 float g(float v,float y)
 {
   return 1./(v*(1.-y)+y);
 }
 void h(inout vec3 v,in vec3 y)
 {
   vec3 x=normalize(y.xyz),f=v;
   float z=dot(f,x);
   f=normalize(v-x*saturate(z)*.5);
   v=f;
 }
 vec4 R(in vec2 v)
 {
   float y=GetDepth(v);
   vec4 f=gbufferProjectionInverse*vec4(v.x*2.f-1.f,v.y*2.f-1.f,2.f*y-1.f,1.f);
   f/=f.w;
   return f;
 }
 vec4 R(in vec2 v,in float y)
 {
   vec4 f=gbufferProjectionInverse*vec4(v.x*2.f-1.f,v.y*2.f-1.f,2.f*y-1.f,1.f);
   f/=f.w;
   return f;
 }
 void G(inout vec3 v,in vec3 y,in vec3 f,vec3 x,float z)
 {
   float r=length(y);
   r*=pow(eyeBrightnessSmooth.y/240.f,6.f);
   r*=rainStrength;
   float i=pow(exp(-r*1e-05),4.);
   i=max(i,.5);
   vec3 c=vec3(dot(colorSkyUp,vec3(1.)))*.05;
   v=mix(c,v,vec3(i));
 }
 vec4 G(float v,float y,vec3 s,vec3 t,vec3 x,vec3 z,vec3 r,float w,float n)
 {
   float m=1.;
   #ifdef SUNLIGHT_LEAK_FIX
   if(isEyeInWater<1)
     m=saturate(n*100.);
   #endif
   v=max(v-.05,0.);
   y=0.;
   float h=v*v,o=fract(frameCounter*.0123456);
   vec3 R=c(texcoord.xy).xyz*.99+.005,a=c(texcoord.xy+.1).xyz,l=reflect(r,c(c(texcoord.xy).xy*vec2(1.,.8),h,x)),M=normalize((gbufferModelView*vec4(l.xyz,0.)).xyz);
   if(dot(l,x)<0.)
     l=reflect(l,x);
   #ifdef REFLECTION_SCREEN_SPACE_TRACING
   bool g=false;
   {
     const int A=16;
     vec2 b=texcoord.xy;
     vec3 T=t.xyz;
     float W=0.;
     vec3 D=t.xyz;
     float P=.1/saturate(dot(-r,x)+.001),U=P*2.,k=1.,Y=0.;
     for(int N=0;
N<A;
N++)
       {
         float S=float(N),u=(S+.5)/float(A);
         vec3 F=M.xyz*P*(.1+length(D)*.1)*k;
         float H=U*(length(D)*.1);
         D+=F;
         vec2 C=ProjectBack(D).xy;
         vec3 E=GetViewPosition(C.xy,GetDepth(C.xy)).xyz;
         float J=length(D)-length(E)-.02;
         if(D.z>0.)
           {
             break;
           }
         if(J>0.&&J<H&&C.x>0.&&C.x<1.&&C.y>0.&&C.y<1.)
           {
             D-=F;
             k*=.5;
             Y+=1.;
             if(Y>2.)
               {
                 g=true;
                 b=C.xy;
                 T=E.xyz;
                 W=distance(D,t.xyz)*.4;
                 break;
               }
           }
       }
     vec3 N=(gbufferModelViewInverse*vec4(T,0.)).xyz;
     if(length(N)>far)
       g=false;
     if(g)
       {
         b.xy=floor(b.xy*vec2(viewWidth,viewHeight)+.5)/vec2(viewWidth,viewHeight);
         TemporalJitterProjPos01(b);
         vec3 F=pow(texture2DLod(colortex3,b.xy,0).xyz,vec3(2.2)),C=F*100.;
         LandAtmosphericScattering(C,T,M,l,worldSunVector,1.);
         G(C,T,normalize(t.xyz),normalize(s.xyz),1.);
         UnderwaterFog(C,length(T),r,colorSkyUp,colorSunlight);
         return vec4(C,saturate(W/4.));
       }
   }
   #endif
   int D=f(),T=e();
   vec3 C=s+x*(.01+w*.1);
   C+=Fract01(cameraPosition.xyz+.5);
   Ray b=MakeRay(e(C,D)*D-vec3(1.),l);
   vec3 A=vec3(1.),W=vec3(0.);
   float N=0.;
   VoxelDDA F=p(b);
   float Y=far;
   vec3 S=vec3(1.);
   for(int J=0;
J<1;
J++)
     {
       vec4 U=vec4(0.);
       vec3 E=vec3(0.);
       float P=.5;
       for(int k=0;
k<REFLECTION_TRACE_LENGTH;
k++)
         {
           E=F.voxelPos/float(D);
           vec2 H=d(E,D);
           U=texture2DLod(shadowcolor,H,0);
           N=U.w*255.;
           float u=1.-step(.5,abs(N-241.));
           vec3 K=U.xyz;
           float L=dot(F.voxelPos+.5-b.origin,F.voxelPos+.5-b.origin),I=saturate(pow(saturate(dot(b.direction,normalize(F.voxelPos+.5-b.origin))),56.*L)*5.-1.)*5.;
           W+=K*u*P*.5*I;
           if(N<240.)
             {
               if(G(F.voxelPos,N,b,k==0,Y,S))
                 {
                   break;
                 }
             }
           i(F);
           P=1.;
         }
       if(U.w*255.<1.f||U.w*255.>254.f)
         {
           vec3 k=SkyShading(b.direction,worldSunVector,rainStrength);
           k=DoNightEyeAtNight(k*12.,timeMidnight)*.083333;
           k=TintUnderwaterDepth(k);
           vec3 H=k*A;
           #ifdef CLOUDS_IN_REFLECTIONS
           CloudPlane(H,-b.direction,worldLightVector,colorSunlight,colorSkyUp,H,timeMidnight,false);
           #endif
           W+=H*.1;
           Y=1000.;
           break;
         }
       vec3 k=mod(b.origin+b.direction*Y,vec3(1.))-.5;
       float H=log2(Y*.4*v*TEXTURE_RESOLUTION);
       vec2 u=vec2(0.);
       u+=vec2(k.z*-S.x,-k.y)*abs(S.x);
       u+=vec2(k.x,k.z*S.y)*abs(S.y);
       u+=vec2(k.x*S.z,-k.y)*abs(S.z);
       vec3 K=(b.origin+b.direction*Y)/float(D);
       vec2 L=textureSize(colortex0,0);
       vec4 I=texture2DLod(shadowcolor1,d(E,D),0);
       vec2 V=I.xy;
       V=(floor(V*L/TEXTURE_RESOLUTION)+.5)/(L/TEXTURE_RESOLUTION);
       vec2 Q=V+u.xy*(TEXTURE_RESOLUTION/L);
       vec3 O=pow(texture2DLod(colortex0,Q,H).xyz,vec3(2.2));
       O*=mix(vec3(1.),U.xyz/(I.w+1e-05),vec3(I.z));
       if(N<240.)
         {
           vec3 B=saturate(U.xyz);
           A*=O;
         }
       if(abs(N-31.)<.1)
         W+=.09*A*GI_LIGHT_BLOCK_INTENSITY;
       if(Y*v>.2)
         {
           vec3 B=vec3(1.)-abs(S);
           W+=c(E+(k+(a.xyz-.5)*2.)/float(D)*B,S)*A;
         }
       else
         {
           vec3 B=vec3(0.),X=vec3(0.);
           if(abs(S.x)>.5)
             B=vec3(0.,1.,0.),X=vec3(0.,0.,1.);
           if(abs(S.y)>.5)
             B=vec3(1.,0.,0.),X=vec3(0.,0.,1.);
           if(abs(S.z)>.5)
             B=vec3(1.,0.,0.),X=vec3(0.,1.,0.);
           B*=1.;
           X*=1.;
           vec3 q=c(E,S),j=q,Z=saturate(q*100000.),ab=c(E+B/float(D),S);
           j+=ab;
           Z+=saturate(ab*100000.);
           vec3 ac=c(E-B/float(D),S);
           j+=ac;
           Z+=saturate(ac*100000.);
           vec3 ad=c(E+X/float(D),S);
           j+=ad;
           Z+=saturate(ad*100000.);
           vec3 ae=c(E-X/float(D),S);
           j+=ae;
           Z+=saturate(ae*100000.);
           j/=Z+vec3(.0001);
           W+=j*A;
         }
       const float B=2.4;
       vec3 X=e(C+b.direction*Y-1.,worldLightVector,S,l,D)*A*B*colorSunlight*m;
       if(isEyeInWater>0)
         ;
       W+=X;
     }
   vec3 k=t.xyz+M*Y,E=(gbufferModelViewInverse*vec4(k.xyz,0.)).xyz;
   if(Y<1000.)
     LandAtmosphericScattering(W,k,M,l,worldSunVector,1.);
   G(W,k,normalize(t.xyz),normalize(s.xyz),1.);
   UnderwaterFog(W,length(E),r,colorSkyUp,colorSunlight);
   Y*=saturate(dot(-r,x))*2.;
   return vec4(W,saturate(Y/4.));
 }
 vec4 T(float v)
 {
   float y=v*v,f=y*v;
   vec4 r;
   r.x=-f+3*y-3*v+1;
   r.y=3*f-6*y+4;
   r.z=-3*f+3*y+3*v+1;
   r.w=f;
   return r/6.f;
 }
 vec4 T(in sampler2D v,in vec2 f)
 {
   vec2 y=vec2(viewWidth,viewHeight);
   f*=y;
   f-=.5;
   float x=fract(f.x),t=fract(f.y);
   f.x-=x;
   f.y-=t;
   vec4 i=T(x),r=T(t),m=vec4(f.x-.5,f.x+1.5,f.y-.5,f.y+1.5),s=vec4(i.x+i.y,i.z+i.w,r.x+r.y,r.z+r.w),c=m+vec4(i.y,i.w,r.y,r.w)/s,z=texture2DLod(v,vec2(c.x,c.z)/y,0),w=texture2DLod(v,vec2(c.y,c.z)/y,0),n=texture2DLod(v,vec2(c.x,c.w)/y,0),d=texture2DLod(v,vec2(c.y,c.w)/y,0);
   float D=s.x/(s.x+s.y),S=s.z/(s.z+s.w);
   return mix(mix(d,n,D),mix(w,z,D),S);
 }
 bool i(vec3 v,vec3 y)
 {
   vec3 x=normalize(cross(dFdx(v),dFdy(v))),t=normalize(y-v),i=normalize(t);
   return distance(v,y)<.05;
 }
 vec3 y(vec2 v)
 {
   vec2 y=vec2(viewWidth,viewHeight),x=1./y,f=v*y,i=floor(f-.5)+.5,t=f-i,z=t*t,r=t*z;
   float m=.5;
   vec2 s=-m*r+2.*m*z-m*t,c=(2.-m)*r-(3.-m)*z+1.,w=-(2.-m)*r+(3.-2.*m)*z+m*t,d=m*r-m*z,n=c+w,S=x*(i+w/n);
   vec3 a=texture2DLod(colortex4,vec2(S.x,S.y),0).xyz;
   vec2 k=x*(i-1.),b=x*(i+2.);
   vec4 D=vec4(texture2DLod(colortex4,vec2(S.x,k.y),0).xyz,1.)*(n.x*s.y)+vec4(texture2DLod(colortex4,vec2(k.x,S.y),0).xyz,1.)*(s.x*n.y)+vec4(a,1.)*(n.x*n.y)+vec4(texture2DLod(colortex4,vec2(b.x,S.y),0).xyz,1.)*(d.x*n.y)+vec4(texture2DLod(colortex4,vec2(S.x,b.y),0).xyz,1.)*(n.x*d.y);
   return max(vec3(0.),D.xyz*(1./D.w));
 }
 void main()
 {
   GBufferData v=GetGBufferData();
   GBufferDataTransparent y=GetGBufferDataTransparent();
   MaterialMask f=CalculateMasks(v.materialID),i=CalculateMasks(y.materialID);
   bool x=y.depth<v.depth;
   if(x)
     v.depth=y.depth,v.normal=y.normal,v.smoothness=y.smoothness,v.metalness=0.,v.mcLightmap=y.mcLightmap,i.sky=0.;
   vec4 r=GetViewPosition(texcoord.xy,v.depth),s=gbufferModelViewInverse*vec4(r.xyz,1.),t=gbufferModelViewInverse*vec4(r.xyz,0.);
   vec3 z=normalize(r.xyz),c=normalize(t.xyz),n=normalize((gbufferModelViewInverse*vec4(v.normal,0.)).xyz),w=normalize((gbufferModelViewInverse*vec4(v.geoNormal,0.)).xyz);
   float m=length(r.xyz);
   vec4 a=vec4(0.);
   float d=G(v.smoothness,v.metalness);
   if(d>.0001&&i.sky<.5)
     a=G(1.-v.smoothness,v.metalness,s.xyz,r.xyz,n.xyz,w,c.xyz,f.leaves,v.mcLightmap.y);
   vec4 k=texture2DLod(colortex3,texcoord.xy,0);
   vec3 b=k.xyz;
   b.xyz=pow(b.xyz,vec3(2.2));
   if(x)
     {
       vec3 S=GetViewPosition(texcoord.xy,texture2DLod(depthtex1,texcoord.xy,0).x).xyz;
       float e=length(S.xyz),D=e-m;
       vec3 C=y.normal-y.geoNormal*1.05;
       float A=saturate(D*.5);
       vec2 l=texcoord.xy+C.xy/(m+1.5)*A;
       {
         float p=ExpToLinearDepth(texture2DLod(depthtex1,l,0).x),E=ExpToLinearDepth(texture2DLod(depthtex0,l,0).x);
         if(E>=p)
           l=texcoord.xy;
       }
       b.xyz=pow(texture2DLod(colortex3,l.xy,0).xyz,vec3(2.2));
       S=GetViewPosition(l.xy,texture2DLod(depthtex1,l.xy,0).x).xyz;
       r=GetViewPosition(l.xy,texture2DLod(depthtex0,l.xy,0).x);
       e=length(S.xyz);
       m=length(r.xyz);
       D=e-m;
       if(i.water>.5&&isEyeInWater<1)
         {
           b.xyz*=exp(GetWaterAbsorption()*-D);
           vec3 E=GetWaterFogColor()*.001;
           E*=exp(-GetWaterAbsorption()*((-c.y*.5+.5)*3.));
           E*=pow(curve(saturate(mix(c.y,1.,.5))),2.)*.6+.4;
           E*=(colorSkyUp+colorSunlight*2.)*1.4;
           E=TintUnderwaterDepth(E);
           float N=exp(-D*.03);
           E*=exp(-GetWaterAbsorption()*(1.-N)*20.);
           E*=eyeBrightnessSmooth.y/240.;
           b=mix(E,b,vec3(N));
         }
       if(i.stainedGlass>.5)
         {
           vec3 h=normalize(y.albedo.xyz+.0001)*pow(length(y.albedo.xyz),.5);
           b.xyz*=mix(vec3(1.),h,vec3(pow(y.albedo.w,.2)));
           b.xyz*=mix(vec3(1.),h,vec3(pow(y.albedo.w,.2)));
         }
     }
   b.xyz=pow(b.xyz,vec3(1./2.2));
   gl_FragData[0]=texture2DLod(colortex0,texcoord.xy,0);
   gl_FragData[1]=vec4(b.xyz,v.smoothness);
   gl_FragData[2]=a*vec4(vec3(.1),1.);
 }

/* RENDERTARGETS: 0,3,6 */
