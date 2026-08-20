// The shadow frustum is view-independent inside a voxelizing pack's declared
// voxel radius.
//
// rethinking-voxels' coloured lighting is fed by a voxel volume, and the only
// thing that writes that volume is geometry drawn into the SHADOW pass
// (shadow.gsh). A section the shadow pass culls is a section whose torches are
// absent from the volume. So inside the radius the pack declares, shadow-pass
// visibility must not depend on where the player is looking — which is what
// `shadow.culling = reversed` buys: Iris' SafeZoneCullingFrustum returns true
// for anything the voxel BoxCuller does not cull, before the view-derived
// planes are consulted at all
// (third_party/iris/.../frustum/advanced/SafeZoneCullingFrustum.java).
//
// These tests read the REAL installed packs, so they also fail if the directives
// they depend on stop being parsed — a voxelDistance that silently reads back
// as 0, or a shadow.culling that stops resolving to SafeZone.
//
#include <gtest/gtest.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <string>
#include <vector>
#include "net/minecraft/client/render/camera/FrameRenderCamera.hpp"
#include "net/minecraft/client/render/culling/ShadowFrustum.hpp"
#include "net/minecraft/client/render/pipeline/Instance.hpp"
#include "net/minecraft/client/render/shaderpack/Catalog.hpp"
#include "net/minecraft/client/render/shaderpack/Loader.hpp"
#include "net/minecraft/client/render/shaderpack/Pack.hpp"
#include "net/minecraft/client/render/shaders/Compiler.hpp"
#include "net/minecraft/client/resource/pack/ZippedTexturePack.hpp"
#include "net/minecraft/client/gl/GLCore.hpp"
#include "net/minecraft/util/math/Matrix4f.hpp"
namespace net::minecraft::test {
namespace {
using client::render::createShadowFrustum;
using client::render::FrameRenderCamera;
using client::render::PackDefinition;
using client::render::PackInstance;
using client::render::PackLoader;
using client::render::ShadowCullingFrustum;
using client::render::ShadowCullState;
using client::render::ShadowFrustumParams;
constexpr float kPi = 3.14159265358979f;
// The engine's own render distance, in blocks — createShadowFrustum reads this
// back for the "leave it to the engine" cases, so the tests have to agree with
// what the section ring is actually built from.
constexpr float kRenderDistanceBlocks = 256.0f;
bool loadPack(std::string_view name,
              PackInstance& pack,
              const std::unordered_map<std::string, std::string>& optionValues = {}) {
 const std::filesystem::path dir = std::filesystem::path(MINECRAFT_TEST_SOURCE_DIR) / "shaders" / name;
 if(!std::filesystem::is_directory(dir)) {
  return false;
 }
 pack.path = dir;
 pack.directory = true;
 const std::vector<std::string> resources = client::render::PackCatalog::directoryResources(dir);
 if(!PackLoader::load(
        resources,
        [&pack](std::string_view path) { return client::render::PackCompiler::readText(pack, std::string(path)); },
        pack.definition, pack.sourceOptions, pack.summary.error, optionValues)) {
  return false;
 }
 pack.rootDefinition = pack.definition;
 return true;
}
// Mirrors Pipeline::selectDimension for the scalars these tests read: the
// dimension definition is seeded from the root at load time, so taking it whole
// keeps every root-declared directive and overlays whatever world0 changed.
const PackDefinition& overworldDefinition(const PackInstance& pack) {
 for(const char* key : {"minecraft:overworld", "*"}) {
  const auto found = pack.rootDefinition.dimensionDefinitions.find(key);
  if(found != pack.rootDefinition.dimensionDefinitions.end() && found->second != nullptr) {
   return *found->second;
  }
 }
 return pack.rootDefinition;
}
PackDefinition activeOverworldDefinition(const PackInstance& pack) {
 PackDefinition definition = overworldDefinition(pack);
 for(const client::render::CustomImage& image : pack.rootDefinition.images) {
  const auto found = std::find_if(definition.images.begin(), definition.images.end(), [&](const client::render::CustomImage& entry) {
   return entry.name == image.name;
  });
  if(found == definition.images.end()) definition.images.push_back(image);
 }
 return definition;
}
// A player camera looking along `yaw` (radians, 0 = +Z), level pitch.
FrameRenderCamera cameraLookingAt(float yaw) {
 FrameRenderCamera camera;
 camera.x = camera.eyeX = 0.0;
 camera.y = camera.eyeY = 70.0;
 camera.z = camera.eyeZ = 0.0;
 const float sinYaw = std::sin(yaw);
 const float cosYaw = std::cos(yaw);
 camera.viewForwardX = sinYaw;
 camera.viewForwardY = 0.0f;
 camera.viewForwardZ = cosYaw;
 camera.viewRightX = cosYaw;
 camera.viewRightY = 0.0f;
 camera.viewRightZ = -sinYaw;
 camera.viewUpX = 0.0f;
 camera.viewUpY = 1.0f;
 camera.viewUpZ = 0.0f;
 camera.nearPlane = 0.05f;
 camera.farPlane = kRenderDistanceBlocks;
 camera.projectionX = 1.0f / std::tan(70.0f * kPi / 360.0f);
 camera.projectionY = camera.projectionX;
 return camera;
}
void playerModelViewProjection(const FrameRenderCamera& camera, float out[16]) {
 float projection[16]{};
 float modelView[16]{};
 client::render::buildCameraProjection(projection, camera);
 client::render::buildCameraModelView(modelView, camera);
 net::minecraft::util::math::Matrix4f composed;
 composed.set(projection);
 net::minecraft::util::math::Matrix4f mv;
 mv.set(modelView);
 composed.multiply(mv);
 std::memcpy(out, composed.data(), sizeof(float) * 16);
}
ShadowFrustumParams paramsFor(const PackDefinition& definition) {
 ShadowFrustumParams params;
 params.cullState = definition.shadowCulling;
 params.packHasVoxelization = [&] {
  const auto found = definition.programs.find("shadow");
  return found != definition.programs.end() && !found->second.geometry.empty();
 }();
 params.halfPlaneLength = definition.shadowDistance;
 params.voxelDistance = definition.effectiveVoxelDistance();
 params.renderMultiplier = definition.shadowDistanceRenderMul;
 params.renderDistanceBlocks = kRenderDistanceBlocks;
 return params;
}
// A 16-block section whose centre sits `distance` blocks from the camera along
// `bearing`, at the camera's own height.
net::minecraft::Box sectionAt(float bearing, double distance) {
 const double cx = std::sin(bearing) * distance;
 const double cz = std::cos(bearing) * distance;
 return net::minecraft::Box{cx - 8.0, 62.0, cz - 8.0, cx + 8.0, 78.0, cz + 8.0};
}
// The sun near the horizon: the extruded planes are at their most directional
// here, so a view-dependent frustum is at its most view-dependent.
const float kLightVector[3] = {0.6f, 0.8f, 0.0f};
class RvoxShadowCullingTest : public ::testing::Test {
 protected:
 // rethinking-voxels declares `iris.features.required = COMPUTE_SHADERS SSBO ...`,
 // and PackLoader::load refuses the pack outright when a required feature is
 // unsupported — which, without a context, every GL-backed feature is. The pack
 // cannot be loaded at all without one.
 static void SetUpTestSuite() {
  if(glfwInit() != GLFW_TRUE) return;
  glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
  window_ = glfwCreateWindow(64, 64, "rvox-culling-test", nullptr, nullptr);
  if(window_ == nullptr) return;
  glfwMakeContextCurrent(window_);
  client::gl::GLCore::ensureLoaded();
 }
 static void TearDownTestSuite() {
  glfwMakeContextCurrent(nullptr);
  if(window_ != nullptr) {
   glfwDestroyWindow(window_);
   window_ = nullptr;
  }
 }
 void SetUp() override {
  if(window_ == nullptr) GTEST_SKIP() << "no GL 4.3 context available";
 }
 static GLFWwindow* window_;
};
GLFWwindow* RvoxShadowCullingTest::window_ = nullptr;
} // namespace
// The load-bearing assertion. Sweep the camera through a full turn and require
// that a section well inside the pack's voxel radius is drawn into the shadow
// pass at EVERY yaw. If this fails, the pack's voxel volume gains and loses
// blocks as the player turns, and its lights arrive late.
TEST_F(RvoxShadowCullingTest, SectionsInsideTheVoxelRadiusAreVisibleAtEveryYaw) {
 PackInstance pack;
 ASSERT_TRUE(loadPack("rethinking-voxels_r0.1-beta9", pack)) << pack.summary.error;
 const PackDefinition definition = activeOverworldDefinition(pack);
 const ShadowFrustumParams params = paramsFor(definition);
 ASSERT_GT(params.voxelDistance, 0.0f)
     << "voxelDistance did not parse; the always-render zone collapsed to nothing";
 // Comfortably inside the voxel radius, and inside a section that straddles it
 // on neither side.
 ASSERT_GT(params.voxelDistance, definition.voxelDistance);
 const double insideDistance = static_cast<double>(params.voxelDistance) * 0.75;
 for(int step = 0; step < 16; ++step) {
  const float yaw = static_cast<float>(step) * (2.0f * kPi / 16.0f);
  float mvp[16]{};
  playerModelViewProjection(cameraLookingAt(yaw), mvp);
  ShadowCullingFrustum frustum = createShadowFrustum(params, mvp, kLightVector);
  frustum.prepare(0.0, 70.0, 0.0);
  for(int bearingStep = 0; bearingStep < 16; ++bearingStep) {
   const float bearing = static_cast<float>(bearingStep) * (2.0f * kPi / 16.0f);
   EXPECT_TRUE(frustum.isVisible(sectionAt(bearing, insideDistance)))
       << "yaw step " << step << ", bearing step " << bearingStep << ": a section " << insideDistance
       << " blocks away was culled from the shadow pass, so its torches leave the voxel volume when the "
          "player looks away";
  }
 }
}
// The same claim stated structurally: inside the voxel radius the answer must
// not depend on the view matrix at all.
TEST_F(RvoxShadowCullingTest, VisibilityInsideTheVoxelRadiusIsIndependentOfTheViewMatrix) {
 PackInstance pack;
 ASSERT_TRUE(loadPack("rethinking-voxels_r0.1-beta9", pack)) << pack.summary.error;
 const ShadowFrustumParams params = paramsFor(activeOverworldDefinition(pack));
 const double insideDistance = static_cast<double>(params.voxelDistance) * 0.5;
 float northMvp[16]{};
 float southMvp[16]{};
 playerModelViewProjection(cameraLookingAt(0.0f), northMvp);
 playerModelViewProjection(cameraLookingAt(kPi), southMvp);
 ShadowCullingFrustum north = createShadowFrustum(params, northMvp, kLightVector);
 ShadowCullingFrustum south = createShadowFrustum(params, southMvp, kLightVector);
 north.prepare(0.0, 70.0, 0.0);
 south.prepare(0.0, 70.0, 0.0);
 for(int bearingStep = 0; bearingStep < 32; ++bearingStep) {
  const float bearing = static_cast<float>(bearingStep) * (2.0f * kPi / 32.0f);
  const net::minecraft::Box box = sectionAt(bearing, insideDistance);
  EXPECT_EQ(north.isVisible(box), south.isVisible(box)) << "bearing step " << bearingStep;
 }
}
// The directives this all rests on, read back off the real pack. RVox declares
// `shadow.culling = reversed` (Iris ShadowCullState.REVERSED, this engine's
// SafeZone) and `const float voxelDistance` in lib/common.glsl; if either stops
// resolving, the frustum silently becomes the ordinary view-derived Advanced one
// and the test above is the only thing that notices.
TEST_F(RvoxShadowCullingTest, PackResolvesToSafeZoneCullingWithAVoxelRadius) {
 PackInstance pack;
 ASSERT_TRUE(loadPack("rethinking-voxels_r0.1-beta9", pack)) << pack.summary.error;
 const PackDefinition definition = activeOverworldDefinition(pack);
 EXPECT_EQ(definition.shadowCulling, ShadowCullState::SafeZone)
     << "shaders.properties says shadow.culling=reversed";
 EXPECT_GT(definition.voxelDistance, 0.0f) << "lib/common.glsl declares const float voxelDistance = 32";
 EXPECT_GT(definition.shadowDistance, 0.0f);
 const ShadowFrustumParams params = paramsFor(definition);
 float mvp[16]{};
 playerModelViewProjection(cameraLookingAt(0.0f), mvp);
 const ShadowCullingFrustum frustum = createShadowFrustum(params, mvp, kLightVector);
 EXPECT_TRUE(frustum.mode() == ShadowCullingFrustum::Mode::SafeZone ||
             frustum.mode() == ShadowCullingFrustum::Mode::NonCulling)
     << "a voxelizing pack must not end up on the view-derived Advanced frustum";
 if(frustum.mode() == ShadowCullingFrustum::Mode::SafeZone) {
 EXPECT_NEAR(frustum.boxCullerDistance(), static_cast<double>(definition.effectiveVoxelDistance()), 1.0e-3)
      << "the always-render zone must be the pack's voxelDistance, not its shadowDistance";
 }
}
// The other half of RVox's light pipeline: shadowcomp/shadowcomp1 are COMPUTE
// passes that sweep the whole 3D voxel volume and register every light they find
// (`registerLight` in program/shadowcomp1.glsl). Their dispatch size comes from a
// `const ivec3 workGroups` declaration — but RVox's .csh files are three lines of
// `#version` / `#define` / `#include`, so that declaration only exists after
// include expansion, and it sits inside an `#if VX_VOL_SIZE ==` branch besides.
//
// If the loader ever reads those files unexpanded, a compute pass silently falls
// dispatches a viewport-sized 2D grid with Z=1 over a 3D volume: only the bottom
// slab is ever visited, so most of the voxel volume never registers its lights
// and torches appear only once the player moves one into the slab.
TEST_F(RvoxShadowCullingTest, VoxelComputePassesDispatchOverTheWholeVolume) {
 PackInstance pack;
 ASSERT_TRUE(loadPack("rethinking-voxels_r0.1-beta9", pack)) << pack.summary.error;
 const PackDefinition& definition = overworldDefinition(pack);
 int checked = 0;
 int sweepsAVolume = 0;
 for(const auto& pass : definition.passes) {
  if(pass.type != "compute" || pass.name.rfind("shadowcomp", 0) != 0) continue;
  ++checked;
  // Every shadowcomp variant in this pack declares `const ivec3 workGroups`
  // in whichever `#ifdef CSH*` branch its .csh selects, so none of them may
  // land on Java's viewport-relative fallback.
  EXPECT_FALSE(pass.relativeGroups)
      << pass.name << ": fell back to a viewport-relative dispatch, so its `const ivec3 workGroups` "
                      "declaration was not seen";
  if(pass.relativeGroups) continue;
  EXPECT_GE(pass.groups[0], 1) << pass.name;
  EXPECT_GE(pass.groups[1], 1) << pass.name;
  EXPECT_GE(pass.groups[2], 1) << pass.name;
  if(pass.groups[2] > 1) ++sweepsAVolume;
 }
 EXPECT_GT(checked, 0) << "no shadowcomp compute passes were registered at all";
 // Not every variant sweeps the volume — shadowcomp_b's non-LIGHT_CLUMPING
 // branch is `const ivec3 workGroups = ivec3(1, 1, 1)` over an empty main().
 // But the ones that build the voxel volume and the light list do, and a Z
 // extent above 1 is reachable only through the absolute branch.
 EXPECT_GT(sweepsAVolume, 0)
     << "no shadowcomp pass dispatches past a single slab; the voxel volume is only partly swept";
}
// Complementary Reimagined guards the same directive behind its own option:
//
//   #if COLORED_LIGHTING > 0
//       shadow.culling = reversed
//
// and lib/common.glsl ships `#define COLORED_LIGHTING 0`. So at stock settings
// it has no voxel volume and correctly gets ordinary Advanced culling — an
// engine that reported SafeZone here would be ignoring the `#if`. Turning the
// option on must flip it, which is what makes the pair a parity test of the
// properties preprocessor rather than of one hardcoded answer.
TEST_F(RvoxShadowCullingTest, ComplementaryShadowCullingFollowsItsColoredLightingOption) {
 PackInstance stock;
 if(!loadPack("ComplementaryReimagined_r5.8.1", stock)) {
  GTEST_SKIP() << "Complementary is not installed in shaders/: " << stock.summary.error;
 }
 EXPECT_EQ(overworldDefinition(stock).shadowCulling, ShadowCullState::Default)
     << "COLORED_LIGHTING defaults to 0, so the `shadow.culling = reversed` line is inside a dead #if";
 PackInstance coloured;
 ASSERT_TRUE(loadPack("ComplementaryReimagined_r5.8.1", coloured, {{"COLORED_LIGHTING", "256"}}))
     << coloured.summary.error;
 EXPECT_EQ(overworldDefinition(coloured).shadowCulling, ShadowCullState::SafeZone)
     << "with COLORED_LIGHTING on, `shadow.culling = reversed` is live and must resolve to SafeZone "
        "(Iris ShadowCullState.REVERSED) — note the spaces around the '=', which the directive parser "
        "has to tolerate";
}
} // namespace net::minecraft::test
