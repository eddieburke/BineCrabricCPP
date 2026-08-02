uniform sampler2D colortex0;
uniform sampler2D colortex1;
uniform sampler2D colortex2;
uniform sampler2D colortex3;
uniform sampler2D colortex4;
uniform sampler2D colortex5;
uniform sampler2D colortex6;
uniform sampler2D colortex7;

#define gaux2 colortex5

uniform sampler2DShadow shadowtex0;
uniform sampler2D shadowtex1;
uniform sampler2D shadowcolor;
uniform sampler2D shadowcolor1;

uniform sampler2D noisetex;
uniform sampler2D depthtex0;
uniform sampler2D depthtex1;

uniform sampler2D gtexture;
#define texture gtexture
uniform sampler2D lightmap;
uniform sampler2D normals;
uniform sampler2D specular;

uniform mat4 gbufferProjection;
uniform mat4 gbufferProjectionInverse;
uniform mat4 gbufferModelView;
uniform mat4 gbufferModelViewInverse;
uniform mat4 shadowProjection;
uniform mat4 shadowProjectionInverse;
uniform mat4 shadowModelView;
uniform mat4 shadowModelViewInverse;
uniform mat4 gbufferPreviousModelView;
uniform mat4 gbufferPreviousProjection;
uniform vec3 sunPosition;
uniform vec3 cameraPosition;
uniform vec3 previousCameraPosition;
uniform vec3 upPosition;

uniform float near;
uniform float far;
uniform float viewWidth;
uniform float viewHeight;
uniform float rainStrength;
uniform float wetness;
uniform float aspectRatio;
uniform int worldTime;
uniform float frameTimeCounter;
uniform float frameTime;
uniform int frameCounter;
uniform float sunAngle;
uniform vec3 skyColor;

uniform int   isEyeInWater;
uniform ivec2 eyeBrightness;
uniform ivec2 eyeBrightnessSmooth;
uniform int   fogMode;

uniform int heldBlockLightValue;
uniform int heldBlockLightValue2;
uniform int heldItemId;
uniform int heldItemId2;

uniform vec4 entityColor; 

uniform float nightVision;
uniform float blindness;

uniform ivec2 atlasSize;

// Time and animation globals
#define ANIMATION_SPEED 1.0f
#define FRAME_TIME (frameTimeCounter * ANIMATION_SPEED)



// #define PHYSICALLY_BASED_MAX_ROUGHNESS // If disabled, materials that are 100% rough will have zero specular highlights (legacy SEUS behavior). If enabled, even 100% rough materials still show some subtle specular highlights. This can cause slowdown if "Full Raytrace Reflections" is on!
#define SUNLIGHT_LEAK_FIX // Fixes sunlight leaking in caves. May cause issues with large outdoor overhanging structures!
#define CAVE_GI_LEAK_FIX // Attempts to fix indirect light leaks from sky and sunlight while in caves by not allowing direct GI from sky or sun on surfaces where the sky light level is zero, and not allowing indirect sky and sun GI at all while the player has been standing in sky light level zero for a while. Large overhanging structures may cause false positives and remove some GI that should be present.


// Shadow map globals
#define SHADOW_MAP_BIAS 0.9



// Surface/texture globals
#define TEXTURE_RESOLUTION 16 // Resolution of current resource pack. This needs to be set properly for reflections! Make sure to use a resource pack with consistent resolution for correct reflections! [4 8 16 32 64 128 256 512 1024 2048]
#define FORCE_WET_EFFECT // Forces all surfaces to get wet when it rains, becoming reflective.
#define RAIN_SPLASH_EFFECT // Rain ripples/splashes on water and wet blocks.
// #define RAIN_SPLASH_BILATERAL // Bilateral filter for rain splash/ripples. When enabled, ripple texture is smoothed (no hard pixel edges) at the cost of performance.
#define WATER_PARALLAX
#define WATER_WAVE_HEIGHT 0.2f
#define SPEC_CHANNEL_SMOOTHNESS r // Which channel (red, green, blue, or alpha) to read from the specular texture to determine smoothness. SEUS uses the red channel by default. [r g b a]
#define SPEC_CHANNEL_METALNESS g // Which channel (red, green, blue, or alpha) to read from the specular texture to determine metalness. SEUS uses the green channel by default [r g b a]
// #define SPEC_SMOOTHNESS_AS_ROUGHNESS // If true, the specular texture channel used to determine smoothness will instead be interpreted as roughness (0 = perfectly smooth, 255 = maximally rough). SEUS uses smoothness (0 = maximally rough, 255 = perfectly smooth) by default.




// Lighting globals
#define TORCHLIGHT_COLOR_TEMPERATURE 3000 // Color temperature of torch light in Kelvin. [1000 1500 2000 2250 2500 2750 3000 3250 3500 3750 4000 4250 4500 4750 5000 5250 5500 5750 6000 6250 6500 6750 7000 7500 8000 8500 9000 10000]
#define HELD_LIGHT // Enables held items to emit light into the scene. The light emitted is always torch-colored for now!
#define NIGHT_BRIGHTNESS (mix(0.00025, 0.002, nightVision))
#define SUNLIGHT_BRIGHTNESS 1.0 // Adjusts the relative brightness of sunlight versus sky/ambient light. [0.2 0.3 0.4 0.5 0.6 0.7 0.8 0.9 1.0 1.1 1.2 1.3 1.4 1.5 1.6 1.7 1.8 1.9 2.0]
#define GODRAYS // Enables volumetric lighting from sunlight (aka "Godrays") while not under water. 
#define GODRAYS_STAINED_GLASS_TINT

// Clouds
#define CUMULUS_CLOUDS // Whether to draw cumulus clouds. Impacts performance, especially if Clouds in GI and/or Clouds in Reflections are enabled
#define CIRRUS_CLOUDS // Whether to draw cirrus clouds. Doesn't impact performance as much as cumulus clouds. Impacts performance, especially if Clouds in GI and/or Clouds in Reflections are enabled
#define CLOUDS_BLOCKY // Whether to draw cumulus clouds in a blocky shape. For that Minecrafty feel.
#define CLOUDS_IN_GI // Whether to consider clouds in GI light tracing. Allows clouds to impact ambient lighting color at the cost of performance.
#define CLOUDS_IN_REFLECTIONS // Whether to draw clouds in reflections. Allows clouds to appear in reflections at the cost of performance.

// Post processing globals
#define TONEMAP_CURVE 2.0 // Controls the intensity of highlights. Lower values give a more filmic look, higher values give a more vibrant/natural look. Default: 5.5 [1.0 1.25 1.5 1.75 2.0 2.25 2.5 2.75 3.0 3.5 4.0 4.5 5.0]
#define EXPOSURE 1.0 // Controls overall brightness/exposure of the image. Higher values give a brighter image. Default: 1.0 [0.4 0.45 0.5 0.55 0.6 0.65 0.7 0.75 0.8 0.85 0.9 0.95 1.0 1.1 1.2 1.3 1.4 1.5 1.75 2.0 2.25 2.5 2.75 3.0 3.5 4.0]
#define GAMMA 1.0 // Gamma adjust. Lower values make shadows darker. Higher values make shadows brighter. Default: 1.0 [0.7 0.725 0.75 0.775 0.8 0.825 0.85 0.875 0.9 0.925 0.95 0.975 1.0 1.025 1.05 1.075 1.1 1.125 1.15 1.175 1.2 1.225 1.25 1.275 1.3]
#define BLACK_DEPTH 0.0 // Adjusts intensity of blacks/low light in the image. Higher values will push darker light into blackness. 0 disables the effect. [0.0 0.2 0.4 0.6 0.8 1.0 1.2 1.4 1.6 1.8 2.0]
#define SATURATION 1.0 // Saturation adjust. Higher values give a more colorful image. Default: 1.0 [0.5 0.6 0.7 0.8 0.9 1.0 1.1 1.2 1.3 1.4 1.5]
#define BLOOM_AMOUNT 1.0 // Amount of bloom to apply to the image. [0.1 0.2 0.3 0.4 0.5 0.6 0.7 0.8 0.9 1.0 1.1 1.2 1.3 1.4 1.5 1.6 1.7 1.8 1.9 2.0]
#define TONEMAP_OPERATOR SEUSTonemap // Each tonemap operator defines a different way to present the raw internal HDR color information to a color range that fits nicely with the limited range of monitors/displays. Each operator gives a different feel to the overall final image. [SEUSTonemap ACESTonemap Uncharted2Tonemap BurgessTonemap ReinhardJodie]
#define MOTION_BLUR // Motion blur. Makes motion look blurry.
// #define MOTION_BLUR_HQ // Higher quality motion blur at a small performance cost
#define MOTION_BLUR_INTENSITY 1.0 // The intensity of motion blur. Higher values will cause more blurring with motion. [0.25 0.5 0.75 1.0 1.5 2.0]
#define POST_SHARPENING 1 // The amount of sharpening to apply to the image before final presentation. [0 1 2 3]



// Utility globals
// #define VISUALIZE_DANGEROUS_LIGHT_LEVEL // When enabled, areas with a block light level of 7 or below (where mobs could spawn) are highlighted in red. Useful for determining the safety of an area in survival gameplay. Doesn't consider sky light level.




// Ray tracing globals
#define GLOWING_REDSTONE_BLOCK // If enabled, redstone blocks are treated as light sources for GI
#define GLOWING_LAPIS_LAZULI_BLOCK // If enabled, lapis lazuli blocks are treated as light sources for GI
#define GLOWING_EMERALD_BLOCK // If enabled, emerald blocks are treated as light sources for GI

// #define FULL_RT_REFLECTIONS // OFF: only surfaces with high smoothness values will render reflections. ON: Allow rougher surfaces to render reflections (higher computational cost). This can cause big slowdowns with lots of semi-rough surfaces!
#define REFLECTION_TRACE_LENGTH 90 // The maximum ray length of ray traced reflections. Higher values result in reflections tracing farther into the world at a higher cost. Lower values can can cause leaking in large indoor areas! [60 90 120 150]
#define DIFFUSE_TRACE_LENGTH 60 // The maximum ray length of path traced diffuse GI. Higher values result in more accurate lighting, taking farther away surfaces into account, at a higher cost. Lower values can cause leaking in large indoor areas! [60 80 100 120]

#define RAYTRACE_GEOMETRY_QUALITY 2 // 0 - No blocks are ray traced with their specual shapes (slabs, stairs, etc.). 1 - Some blocks are traced with special shapes (stairs, slabs, doors, fences). 2 - All blocks (that have been coded so far) are traced with their special shapes. Lower setting causes artifacts but is faster. [0 1 2]

#define GI_RESPONSIVENESS 3 // How well GI responds to lighting changes over time. Lower settings sacrifice responsiveness for improved stability, better noise reduction, and higher detail in GI. [0 1 2 3]
// #define GI_SCREEN_SPACE_TRACING // Allows for surfaces in screen-space to cast indirect shadows and lighting that wouldn't otherwise (mobs, grass, etc.). If enabled, can leave weird ghosty shadows when GI Responsiveness is low.
#define GI_FILTER_QUALITY 1 // Quality of GI filter. [0 1]
// #define REFLECTION_SCREEN_SPACE_TRACING // Allows for surfaces in screen-space to show up in reflections that wouldn't othwerise (mobs, grass, etc.). 
#define GI_SECONDARY_SAMPLES 1 // How many samples to trace for the secondary GI tracing. Higher samples reduces the "disco floor" artifact mostly seen in reflections at the cost of performance. [1 2 3 4]

#define GI_LIGHT_BLOCK_INTENSITY 1.0 // How bright light from solid blocks is. [0.2 0.4 0.6 0.8 1.0 1.5 2.0 2.5 3.0 4.0 5.0 7.0 10.0]
#define GI_LIGHT_TORCH_INTENSITY 1.0 // How bright light from torches is. [0.2 0.4 0.6 0.8 1.0 1.5 2.0 2.5 3.0 4.0 5.0 7.0 10.0]

#define GI_LEAF_TRANSPARENCY // Allows leaves to partially transmit lighting in GI, instead of fully blocking it like a full solid cube. May impact performance in certain situations.

#define GI_SUNLIGHT_STAINED_GLASS_TINT

// #define GI_SIMPLE_SKY_TERM // Off: Provides true ambient GI light color from sky. On: Uses a simpler less-accurate sky term for GI for better performance








// GI volume material IDs










// Special geometry IDs



















































//End of quality 1






































//End of quality 2


















