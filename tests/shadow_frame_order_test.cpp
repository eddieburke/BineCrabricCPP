// Why the shadow map must be rendered BEFORE the frame uniforms are consumed.
//
// shadowModelView/shadowProjection are a pure function of THIS frame's camera
// (ShadowMapPass.hpp). These tests show how sharply they move with the camera,
// which is what makes pairing them with a shadow map rendered on the PREVIOUS
// frame visible as lighting that swims when the player looks around or moves.
#include <gtest/gtest.h>
#include <algorithm>
#include <cmath>
#include "net/minecraft/client/render/camera/FrameRenderCamera.hpp"
#include "net/minecraft/client/render/celestial/CelestialState.hpp"
#include "net/minecraft/client/render/shaderpack/Pack.hpp"
#include "net/minecraft/client/render/targets/ShadowMapPass.hpp"
#include "net/minecraft/client/render/uniforms/FrameData.hpp"
#include "net/minecraft/client/render/uniforms/Uniforms.hpp"
namespace net::minecraft::test {
namespace {
using client::render::CelestialState;
using client::render::FrameRenderCamera;
using client::render::PackDefinition;
using client::render::PackUniformValues;
PackDefinition shadowingPack() {
 PackDefinition definition;
 definition.shadowMapResolution = 2048;
 definition.shadowDistance = 160.0f;
 definition.shadowNearPlane = -160.0f;
 definition.shadowFarPlane = 160.0f;
 return definition;
}
FrameRenderCamera cameraAt(double x, double y, double z) {
 FrameRenderCamera camera;
 camera.x = x;
 camera.y = y;
 camera.z = z;
 camera.eyeX = x;
 camera.eyeY = y;
 camera.eyeZ = z;
 return camera;
}
PackUniformValues frameFor(const FrameRenderCamera& camera) {
 const PackDefinition definition = shadowingPack();
 const CelestialState celestial = client::render::makeCelestialState(0.3f, 0.0f);
 const FrameRenderCamera shadowCamera =
     client::render::shadowmap::makeShadowCamera(definition, camera, celestial);
 return client::render::buildShaderFrameData(1280, 720, 100.0f, definition.shadowMapResolution, false,
                                             true, camera, shadowCamera, nullptr, 10.0f);
}
} // namespace
// The shadow camera is anchored on the player's eye, so any translation moves
// the shadow model-view. A shadow map rendered one frame earlier was rendered
// from the OTHER matrix in this pair.
TEST(ShadowFrameOrder, ShadowModelViewTracksCameraTranslation) {
 const PackDefinition definition = shadowingPack();
 const CelestialState celestial = client::render::makeCelestialState(0.3f, 0.0f);
 const FrameRenderCamera before = cameraAt(100.0, 70.0, -40.0);
 const FrameRenderCamera after = cameraAt(104.5, 70.0, -40.0); // ~one sprint tick
 const FrameRenderCamera shadowBefore =
     client::render::shadowmap::makeShadowCamera(definition, before, celestial);
 const FrameRenderCamera shadowAfter =
     client::render::shadowmap::makeShadowCamera(definition, after, celestial);
 ASSERT_TRUE(shadowBefore.hasExplicitModelView);
 ASSERT_TRUE(shadowAfter.hasExplicitModelView);
 const bool identical = std::equal(std::begin(shadowBefore.explicitModelView),
                                   std::end(shadowBefore.explicitModelView),
                                   std::begin(shadowAfter.explicitModelView));
 EXPECT_FALSE(identical) << "shadow model view must follow the camera; if it did not, a one-frame "
                            "stale shadow map would be harmless and this ordering would not matter";
}
// Same camera, same celestial state => byte-identical matrices. This is the
// property that makes the lag visible only while moving: stand still and the
// stale map happens to agree, so the flicker stops.
TEST(ShadowFrameOrder, StationaryCameraProducesIdenticalShadowMatrices) {
 const PackDefinition definition = shadowingPack();
 const CelestialState celestial = client::render::makeCelestialState(0.3f, 0.0f);
 const FrameRenderCamera camera = cameraAt(100.0, 70.0, -40.0);
 const FrameRenderCamera first =
     client::render::shadowmap::makeShadowCamera(definition, camera, celestial);
 const FrameRenderCamera second =
     client::render::shadowmap::makeShadowCamera(definition, camera, celestial);
 EXPECT_TRUE(std::equal(std::begin(first.explicitModelView), std::end(first.explicitModelView),
                        std::begin(second.explicitModelView)));
}
// The shadow uniforms published to the pack move with the camera too, not just
// the internal camera struct.
TEST(ShadowFrameOrder, PublishedShadowModelViewMovesWithCamera) {
 const PackUniformValues before = frameFor(cameraAt(100.0, 70.0, -40.0));
 const PackUniformValues after = frameFor(cameraAt(104.5, 70.0, -40.0));
 const bool identical = std::equal(std::begin(before.shadowModelView), std::end(before.shadowModelView),
                                   std::begin(after.shadowModelView));
 EXPECT_FALSE(identical);
}
// buildShaderFrameData rolls previous->current and advances the frame counter
// as a side effect, so it may be called exactly once per frame. This is why
// the shadow-availability inputs have to be derived from the pack definition
// up front rather than by rebuilding the block after the shadow pass.
TEST(ShadowFrameOrder, FrameDataMayOnlyBeBuiltOncePerFrame) {
 const FrameRenderCamera camera = cameraAt(0.0, 70.0, 0.0);
 const PackUniformValues first = frameFor(camera);
 const PackUniformValues second = frameFor(camera);
 EXPECT_EQ(second.frameCounter, (first.frameCounter + 1) % 720720)
     << "a second build in the same frame would skip a frameCounter value";
 // The roll must carry the prior call's matrices forward exactly.
 EXPECT_TRUE(std::equal(std::begin(first.gbufferModelView), std::end(first.gbufferModelView),
                        std::begin(second.gbufferPreviousModelView)));
}
// A pack that declares no shadow map must report no shadow, and must not
// publish a nonzero shadowMapResolution for the pack to divide by.
TEST(ShadowFrameOrder, NoShadowPackReportsNoShadowResolution) {
 FrameRenderCamera camera = cameraAt(0.0, 70.0, 0.0);
 FrameRenderCamera shadowCamera;
 const PackUniformValues values = client::render::buildShaderFrameData(
     1280, 720, 100.0f, 0, false, false, camera, shadowCamera, nullptr, 10.0f);
 EXPECT_FLOAT_EQ(values.shadowMapResolution, 0.0f);
}
} // namespace net::minecraft::test
