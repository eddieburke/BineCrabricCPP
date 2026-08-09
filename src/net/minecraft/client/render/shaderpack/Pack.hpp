#pragma once
#include <cstddef>
#include <memory>
#include <string>
#include <set>
#include <unordered_map>
#include <vector>
#include "net/minecraft/client/render/culling/ShadowFrustum.hpp"
#include "net/minecraft/client/render/shaders/CustomUniforms.hpp"
namespace net::minecraft::client::render {
enum class SettingType {
 Int,
 Bool,
 Float,
 Enum
};
struct PackSetting {
 std::string key;
 SettingType type = SettingType::Bool;
 std::string label;
 std::string comment;
 std::string valuePrefix;
 std::string valueSuffix;
 std::unordered_map<std::string, std::string> valueLabels;
 std::vector<std::string> valueOrder;
 double minimum = 0.0;
 double maximum = 1.0;
 double step = 0.01;
 std::string defaultValue;
 bool asSlider = false;
};
struct PackProfile {
 std::string name;
 std::unordered_map<std::string, std::string> values;
 std::vector<std::string> disabledPrograms;
};
struct PackProgramSource {
 std::string vertex;
 std::string fragment;
 std::string compute;
 std::string geometry;
 std::string tessControl;
 std::string tessEvaluation;
};
struct PackTarget {
 std::string format = "RGBA8";
 float scale = 1.0f;
 float scaleX = 1.0f;
 float scaleY = 1.0f;
 int absoluteWidth = 0;
 int absoluteHeight = 0;
 bool clear = true;
 bool mipmap = false;
 bool customClearColor = false;
 float clearColor[4]{};
};
struct ProgramScale {
 float scale = 1.0f;
 float offsetX = 0.0f;
 float offsetY = 0.0f;
};
struct AlphaTestDirective {
 std::string program;
 bool enabled = true;
 std::string func = "ALWAYS";
 float ref = 0.0f;
};
struct CustomTexture {
 std::string name;
 std::string stage;
 std::string path;
 std::string type;
 std::string internalFormat;
 std::vector<int> dimensions;
 std::string pixelFormat;
 std::string pixelType;
 bool encoded = false;
};
struct BufferObject {
 int index = 0;
 std::size_t byteSize = 0;
 bool relative = false;
 float scaleX = 1.0f;
 float scaleY = 1.0f;
 std::string initPath;
};
struct CustomImage {
 std::string name;
 std::string sampler;
 std::string format;
 std::string internalFormat;
 std::string pixelType;
 bool clearEachFrame = false;
 bool relative = false;
 float width = 1.0f;
 float height = 1.0f;
 int depth = 1;
};
struct BufferBlend {
 std::string program;
 int buffer = -1;
 bool enabled = true;
 std::string source;
 std::string destination;
 std::string sourceAlpha;
 std::string destinationAlpha;
};
struct IndirectDispatch {
 int buffer = -1;
 std::size_t offset = 0;
};
struct PackPass {
 std::string name;
 std::string type;
 std::string program;
 std::vector<std::string> outputs;
 std::vector<std::string> mipmapBuffers;
 int groups[3] = {1, 1, 1};
 int localSize[3] = {1, 1, 1};
 float groupScale[2] = {1.0f, 1.0f};
 bool relativeGroups = true;
 int iterations = 1;
 int order = 0;
};
// What the pack declares in shaders.properties / .vsh / .fsh, resolved and stored in
// one place. Fields are grouped by the shaders.properties section they come from.
struct PackDefinition {
 // --- <profile>/VERSION ---
 std::string name = "Shader pack";
 std::string version;
 int glslVersionMajor = 1;
 int glslVersionMinor = 2;
 int shadowMapResolution = 0;
 int shadowColorBuffers = 0;
 int gbufferColorBuffers = 1;
 int noiseTextureResolution = 256;
 int mcMipmapLevel = 0;
 // --- shadow.enabled / shadow* ---
 bool shadowEnabled = true;
 bool shadowPlayer = false;
 bool shadowEntities = true;
 bool shadowTerrain = true;
 bool shadowTranslucent = true;
 bool shadowBlockEntities = true;
 bool shadowLightBlockEntities = false;
 bool skipAllRendering = false;
 bool allowConcurrentCompute = false;
 bool supportsColorCorrection = false;
 bool oldHandLight = true;
 bool dynamicHandLight = false;
 bool dhShadowEnabled = false;
 bool prepareBeforeShadow = false;
 bool breaksAnisotropy = false;
 int fallbackTex = 0;
 ShadowCullState shadowCulling = ShadowCullState::Default;
 bool voxelizeLightBlocks = false;
 bool separateEntityDraws = false;
 bool oldLighting = false;
 bool separateAo = false;
 // ColorWheel texture format (lab-pbr) flags, driven by the selected texture
 // pack's texture.properties format line.
 bool labPbr = false;
 bool labPbr13 = false;
 float ambientOcclusionLevel = 1.0f;
 // --- SM_DIST / shadowDistance ---
 // The pack's own shadow tuning, passed verbatim to makeShadowCamera and the shadow
 // frustum. These describe the SHADOW map only: the camera far plane and the
 // render-distance uniforms come from option::RenderDistance, never from here.
 float entityShadowDistanceMul = 1.0f;
 float voxelDistance = 0.0f;
 float shadowDistance = 160.0f;
 float shadowDistanceRenderMul = -1.0f;
 float shadowMapFov = 0.0f;
 float shadowNearPlane = -100.05f;
 float shadowFarPlane = 156.0f;
 float shadowIntervalSize = 2.0f;
 float sunPathRotation = 0.0f;
  // --- HALF_LIFE ---
  float wetnessHalflife = 600.0f;
  float drynessHalflife = 200.0f;
  float centerDepthHalflife = 1.0f;
  // True when any program source references centerDepthSmooth (or the pack
  // declares const float centerDepthHalflife). The per-frame depth readback
  // that feeds the uniform only runs for packs that actually consume it.
  bool usesCenterDepthSmooth = false;
 float eyeBrightnessHalflife = 10.0f;
 // --- tex. / filter ---
 bool shadowtexNearest[2] = {false, false};
 bool shadowcolorMipmap[8] = {false, false, false, false, false, false, false, false};
 bool shadowcolorNearest[8] = {false, false, false, false, false, false, false, false};
 // --- render.* ---
 bool renderClouds = true;
 std::string cloudsMode;
 std::string dhCloudsMode;
 bool renderSun = true;
 bool renderMoon = true;
 bool renderSky = true;
 bool renderStars = true;
 bool renderWeather = true;
 bool renderWeatherParticles = true;
 bool underwaterOverlay = true;
 bool vignette = true;
 bool endFlashShadows = false;
 bool backFaceSolid = true;
 bool backFaceCutout = true;
 bool backFaceCutoutMipped = true;
 bool backFaceTranslucent = true;
 bool frustumCulling = true;
 bool occlusionCulling = true;
 bool rainDepth = false;
 bool beaconBeamDepth = false;
 bool shadowHardwareFiltering[2] = {false, false};
 bool shadowHardwareOffset = false;
 float shadowHardwareOffsetFactor = 2.0f;
 float shadowHardwareOffsetUnits = 4.0f;
 bool usesWaterShadow = false;
 std::string particleOrdering;
 // --- iris.features.* ---
 // std::less<> so featureEnabled() can look up a string_view without building a
 // std::string, and without degrading to a linear scan to do it.
 std::set<std::string, std::less<>> requiredFeatures;
 std::set<std::string, std::less<>> optionalFeatures;
 // --- dimension.* / entity.* / item.* / block.* ---
 std::unordered_map<std::string, std::string> dimensionFolders;
 std::unordered_map<std::string, int> entityIds;
 std::unordered_map<std::string, int> itemIds;
 std::unordered_map<std::string, int> blockIds;
 bool hasBlockProperties = false;
 std::unordered_map<int, int> blockRenderLayers;
 // --- program.* / blend.* / scale.* ---
 std::unordered_map<std::string, std::string> programEnabled;
 std::unordered_map<std::string, ProgramScale> programScales;
 std::vector<AlphaTestDirective> alphaTests;
 std::unordered_map<std::string, std::shared_ptr<PackDefinition>> dimensionDefinitions;
 std::vector<PackSetting> settings;
 std::vector<std::string> screenRoot;
 std::unordered_map<std::string, std::vector<std::string>> screenPages;
 int screenColumns = 0;
 std::unordered_map<std::string, int> screenPageColumns;
 std::set<std::string> sliderKeys;
 std::vector<PackProfile> profiles;
 // --- resolved sources ---
 std::unordered_map<std::string, PackProgramSource> programs;
 std::unordered_map<std::string, PackTarget> targets;
 std::unordered_map<std::string, bool> flips;
 std::unordered_map<std::string, IndirectDispatch> indirectDispatches;
 std::vector<CustomTexture> customTextures;
 std::vector<BufferObject> bufferObjects;
 std::vector<CustomImage> images;
 std::vector<BufferBlend> bufferBlends;
 std::vector<PackPass> passes;
 std::vector<CustomUniformDecl> customUniforms;
};
[[nodiscard]] inline const PackDefinition& vanillaPackDefinition() noexcept {
 static const PackDefinition vanilla{};
 return vanilla;
}
} // namespace net::minecraft::client::render
