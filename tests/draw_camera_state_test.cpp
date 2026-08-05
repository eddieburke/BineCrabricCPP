// Draw camera state (the single matrix source after the matrix-stack collapse).
// Pure CPU: exercises setDrawCameraState / setDrawCameraStateFromCamera /
// setPassModelView / setDrawPose / ScopedDrawCameraState / gui_proj ortho.
#include <gtest/gtest.h>
#include <cmath>
#include <cstring>
#include "net/minecraft/client/render/camera/FrameRenderCamera.hpp"
#include "net/minecraft/client/render/camera/GuiProjection.hpp"
#include "net/minecraft/client/render/RenderCore.hpp"
#include "net/minecraft/util/math/Matrix4f.hpp"
namespace net::minecraft::test {
namespace {
using net::minecraft::util::math::Matrix4f;
using net::minecraft::client::render::FrameRenderCamera;
namespace core = net::minecraft::client::render::core;
namespace gui_proj = net::minecraft::client::render::gui_proj;

bool nearMatrix(const Matrix4f& a, const Matrix4f& b, float eps = 1e-5f) {
 for(int i = 0; i < 16; ++i) {
  if(std::abs(a.m[i] - b.m[i]) > eps) {
   return false;
  }
 }
 return true;
}
} // namespace
TEST(DrawCameraState, GuiOrthoLoadPublishesCameraState) {
 core::clearDrawCameraState();
 gui_proj::load(854.0f, 480.0f, -2000.0f, 1000.0f, 3000.0f);
 ASSERT_TRUE(core::drawCameraStateValid());
 // modelView = translate(0, 0, modelViewZ).
 const Matrix4f& modelView = core::drawModelView();
 EXPECT_NEAR(modelView.m[0], 1.0f, 1e-5f);
 EXPECT_NEAR(modelView.m[5], 1.0f, 1e-5f);
 EXPECT_NEAR(modelView.m[10], 1.0f, 1e-5f);
 EXPECT_NEAR(modelView.m[12], 0.0f, 1e-5f);
 EXPECT_NEAR(modelView.m[13], 0.0f, 1e-5f);
 EXPECT_NEAR(modelView.m[14], -2000.0f, 1e-5f);
 // projection = ortho(0, rawW, rawH, 0, near, far).
 const Matrix4f& projection = core::drawProjection();
 EXPECT_NEAR(projection.m[0], 2.0f / 854.0f, 1e-5f);
 EXPECT_NEAR(projection.m[5], -2.0f / 480.0f, 1e-5f);
 EXPECT_NEAR(projection.m[10], -2.0f / 2000.0f, 1e-5f);
 EXPECT_NEAR(projection.m[12], -1.0f, 1e-5f);
 EXPECT_NEAR(projection.m[13], 1.0f, 1e-5f);
 EXPECT_NEAR(projection.m[14], -2.0f, 1e-5f);
 // Screen-center pixel maps to the NDC origin through proj * modelView.
 Matrix4f clip = projection * modelView;
 float x = 0.0f;
 float y = 0.0f;
 float z = 0.0f;
 clip.transformPoint(427.0f, 240.0f, 0.0f, x, y, z);
 EXPECT_NEAR(x, 0.0f, 1e-4f);
 EXPECT_NEAR(y, 0.0f, 1e-4f);
 EXPECT_NEAR(z, 0.0f, 1e-4f);
 // GUI camera sits at the origin.
 const float* cameraPosition = core::drawCameraPosition();
 EXPECT_EQ(cameraPosition[0], 0.0f);
 EXPECT_EQ(cameraPosition[1], 0.0f);
 EXPECT_EQ(cameraPosition[2], 0.0f);
}
// The world pass base is gbufferModelView and NOTHING else. It used to carry an
// extra T(cameraEntityPos - eye) so that beta-style producers emitting
// camera-entity-relative geometry landed correctly; that convention is gone, and
// with it the offset. Packs reconstruct world space as
// gbufferModelViewInverse * viewPos + cameraPosition, which only holds when the
// uploaded matrix is anchored on the same point cameraPosition reports.
TEST(DrawCameraState, FromCameraBuildsGbufferModelViewAlone) {
 core::clearDrawCameraState();
 FrameRenderCamera camera;
 camera.x = 100.0;
 camera.y = 200.0;
 camera.z = 300.0;
 camera.eyeX = 112.0;
 camera.eyeY = 190.0;
 camera.eyeZ = 315.0;
 camera.viewRightX = 0.0f;
 camera.viewRightY = 0.0f;
 camera.viewRightZ = -1.0f;
 camera.viewUpX = 1.0f;
 camera.viewUpY = 0.0f;
 camera.viewUpZ = 0.0f;
 camera.viewForwardX = 0.0f;
 camera.viewForwardY = -1.0f;
 camera.viewForwardZ = 0.0f;
 camera.projectionX = 1.2f;
 camera.projectionY = 1.1f;
  camera.perspectiveNear = 0.05f;
  camera.perspectiveFar = 512.0f;
  core::setDrawCameraStateFromCamera(camera);
 ASSERT_TRUE(core::drawCameraStateValid());
 // The base is the pure camera rotation: eye-relative p maps by R alone, with no
 // translation folded in. R here: col0=(0,1,0), col1=(0,0,1), col2=(-1,0,0),
 // so R*p = (-pz, px, py).
 const Matrix4f& modelView = core::drawModelView();
 const float px = 4.0f;
 const float py = -2.0f;
 const float pz = 7.0f;
 float outX = 0.0f;
 float outY = 0.0f;
 float outZ = 0.0f;
 modelView.transformPoint(px, py, pz, outX, outY, outZ);
 EXPECT_NEAR(outX, -pz, 1e-4f);
 EXPECT_NEAR(outY, px, 1e-4f);
 EXPECT_NEAR(outZ, py, 1e-4f);
 // Identical to buildCameraModelView, by construction.
 float gbufferModelView[16]{};
 net::minecraft::client::render::buildCameraModelView(gbufferModelView, camera);
 Matrix4f expected;
 expected.set(gbufferModelView);
 EXPECT_TRUE(nearMatrix(modelView, expected));
 // Projection matches the camera projection build.
 const Matrix4f& projection = core::drawProjection();
 EXPECT_NEAR(projection.m[0], 1.2f, 1e-5f);
 EXPECT_NEAR(projection.m[5], 1.1f, 1e-5f);
 EXPECT_NEAR(projection.m[11], -1.0f, 1e-5f);
 // The camera position published is Java's Camera.getPosition().
 const float* cameraPosition = core::drawCameraPosition();
 EXPECT_NEAR(cameraPosition[0], 112.0f, 1e-3f);
 EXPECT_NEAR(cameraPosition[1], 190.0f, 1e-3f);
 EXPECT_NEAR(cameraPosition[2], 315.0f, 1e-3f);
}
// Iris moves bobbing/nausea out of the projection matrix and LEFT-multiplies them onto
// the model view: `modelViewMatrix.mulLocal(bobStack)` — "need `bob * modelView` not
// `modelView * bob`". The bob therefore acts in view space AFTER the camera and never
// moves Camera.getPosition(), which stays the single geometry origin.
// https://github.com/IrisShaders/Iris/blob/26.1/common/src/main/java/net/irisshaders/iris/mixin/MixinModelViewBobbing.java
TEST(DrawCameraState, BobIsLeftMultipliedAndNeverMovesTheOrigin) {
 core::clearDrawCameraState();
 FrameRenderCamera camera;
 camera.x = 100.0;
 camera.y = 200.0;
 camera.z = 300.0;
 camera.eyeX = 100.0;
 camera.eyeY = 201.62;
 camera.eyeZ = 300.0;
 // A quarter turn about +Y as the camera rotation: R maps world (x,y,z) -> (-z,y,x).
 camera.viewRightX = 0.0f;
 camera.viewRightY = 0.0f;
 camera.viewRightZ = -1.0f;
 camera.viewUpX = 0.0f;
 camera.viewUpY = 1.0f;
 camera.viewUpZ = 0.0f;
 camera.viewForwardX = -1.0f;
 camera.viewForwardY = 0.0f;
 camera.viewForwardZ = 0.0f;
 // bobStack: a pure view-space translation of (+0.05, -0.03, 0).
 Matrix4f bob;
 bob.translate(0.05f, -0.03f, 0.0f);
 std::memcpy(camera.bobModelView, bob.m, sizeof(camera.bobModelView));
  camera.hasBobModelView = true;
  core::setDrawCameraStateFromCamera(camera);
 ASSERT_TRUE(core::drawCameraStateValid());
 // The published origin is Camera.getPosition(), bob or no bob.
 const float* cameraPosition = core::drawCameraPosition();
 EXPECT_NEAR(cameraPosition[0], 100.0f, 1e-3f);
 EXPECT_NEAR(cameraPosition[1], 201.62f, 1e-3f);
 EXPECT_NEAR(cameraPosition[2], 300.0f, 1e-3f);
 // gbufferModelView = bob * R. Camera-relative (4, 1, -7) rotates to (7, 1, 4) and the
 // bob then shifts it in VIEW space. Had the bob been right-multiplied (bob applied in
 // world space before the rotation) the offset would land on different axes — that is
 // the distinction this test pins.
 float gbufferModelView[16]{};
 net::minecraft::client::render::buildCameraModelView(gbufferModelView, camera);
 Matrix4f modelViewMatrix;
 modelViewMatrix.set(gbufferModelView);
 float outX = 0.0f;
 float outY = 0.0f;
 float outZ = 0.0f;
 modelViewMatrix.transformPoint(4.0f, 1.0f, -7.0f, outX, outY, outZ);
 EXPECT_NEAR(outX, 7.0f + 0.05f, 1e-4f);
 EXPECT_NEAR(outY, 1.0f - 0.03f, 1e-4f);
 EXPECT_NEAR(outZ, 4.0f, 1e-4f);
 // The inverse round-trips, so `gbufferModelViewInverse * viewPos` is exactly
 // `worldPos - cameraPosition` and packs reconstruct world space without drift.
 float gbufferModelViewInverse[16]{};
 net::minecraft::client::render::buildCameraModelViewInverse(gbufferModelViewInverse, camera);
 Matrix4f inverse;
 inverse.set(gbufferModelViewInverse);
 inverse.transformPoint(outX, outY, outZ, outX, outY, outZ);
 EXPECT_NEAR(outX, 4.0f, 1e-4f);
 EXPECT_NEAR(outY, 1.0f, 1e-4f);
 EXPECT_NEAR(outZ, -7.0f, 1e-4f);
 // Direction uniforms (sunPosition/upPosition) pick up the bob's rotation but not its
 // translation, matching Java's w=0 transform by gbufferModelView.
 float up[3] = {0.0f, 0.0f, 0.0f};
 net::minecraft::client::render::directionToView(0.0f, 1.0f, 0.0f, camera, up);
 EXPECT_NEAR(up[0], 0.0f, 1e-4f);
 EXPECT_NEAR(up[1], 1.0f, 1e-4f);
 EXPECT_NEAR(up[2], 0.0f, 1e-4f);
}
// A pose is composed and published; it never touches the pass base. The GUI is
// the same path as the world here — that is the whole point of the collapse.
TEST(DrawCameraState, PoseComposesWithoutTouchingThePassBase) {
 core::clearDrawCameraState();
 gui_proj::load(854.0f, 480.0f);
 const Matrix4f base = core::drawModelView();
 Matrix4f pose;
 pose.translate(10.0f, -20.0f, 0.0f);
 core::setDrawPose(pose);
 EXPECT_NEAR(core::drawPose().m[12], 10.0f, 1e-5f);
 EXPECT_NEAR(core::drawPose().m[13], -20.0f, 1e-5f);
 // The pass base is untouched by any number of pose publishes.
 EXPECT_TRUE(nearMatrix(core::drawModelView(), base));
 EXPECT_NEAR(core::drawProjection().m[0], 2.0f / 854.0f, 1e-5f);
 EXPECT_TRUE(core::drawCameraStateValid());
}
// setPassModelView is the pass owner's call: it replaces the base and drops the
// pose, which is meaningless against a base it was not composed against.
TEST(DrawCameraState, PassModelViewReplacesBaseAndDropsPose) {
 core::clearDrawCameraState();
 gui_proj::load(854.0f, 480.0f);
 Matrix4f pose;
 pose.translate(3.0f, 4.0f, 5.0f);
 core::setDrawPose(pose);
 Matrix4f handBase;
 handBase.translate(0.0f, 0.0f, -7.0f);
 core::setPassModelView(handBase);
 EXPECT_TRUE(nearMatrix(core::drawModelView(), handBase));
 EXPECT_TRUE(nearMatrix(core::drawPose(), Matrix4f::identityMatrix()));
}
TEST(DrawCameraState, ScopedDrawCameraStateRestoresBase) {
 core::clearDrawCameraState();
 gui_proj::load(854.0f, 480.0f);
 const Matrix4f savedModelView = core::drawModelView();
 const Matrix4f savedProjection = core::drawProjection();
 const float savedCameraPosition[3] = {core::drawCameraPosition()[0],
                                       core::drawCameraPosition()[1],
                                       core::drawCameraPosition()[2]};
 {
  const core::ScopedDrawCameraState guard;
  FrameRenderCamera camera;
  camera.x = camera.y = camera.z = 0.0;
   camera.eyeX = 1.0;
   camera.eyeY = 2.0;
   camera.eyeZ = 3.0;
   core::setDrawCameraStateFromCamera(camera);
  EXPECT_NEAR(core::drawCameraPosition()[0], 1.0f, 1e-3f);
 }
 ASSERT_TRUE(core::drawCameraStateValid());
 EXPECT_TRUE(nearMatrix(core::drawModelView(), savedModelView));
 EXPECT_TRUE(nearMatrix(core::drawProjection(), savedProjection));
 const float* cameraPosition = core::drawCameraPosition();
 EXPECT_NEAR(cameraPosition[0], savedCameraPosition[0], 1e-6f);
 EXPECT_NEAR(cameraPosition[1], savedCameraPosition[1], 1e-6f);
 EXPECT_NEAR(cameraPosition[2], savedCameraPosition[2], 1e-6f);
}
// The pose is state, not a token consumed by the next draw. A model publishes
// one bone pose and emits several Tessellator batches from it; when draw()
// cleared the pose, every part after the first rendered with no pose at all —
// i.e. at the camera, with the entity translation gone.
TEST(DrawCameraState, PoseSurvivesRepeatedDraws) {
 core::clearDrawCameraState();
 FrameRenderCamera camera;
 camera.eyeX = 10.0;
 camera.eyeY = 20.0;
 camera.eyeZ = 30.0;
 core::setDrawCameraStateFromCamera(camera);
 // A fresh pass starts at identity: geometry is already in pass-base space.
 EXPECT_TRUE(nearMatrix(core::drawPose(), Matrix4f::identityMatrix()));
 Matrix4f pose;
 pose.translate(4.0f, 5.0f, 6.0f);
 core::setDrawPose(pose);
 // Stand-in for what Tessellator::draw()/reset() does between batches.
 for(int batch = 0; batch < 3; ++batch) {
  EXPECT_TRUE(nearMatrix(core::drawPose(), pose)) << "batch " << batch;
 }
}
TEST(DrawCameraState, PoseIsScopedAndClearedAtPassBoundaries) {
 core::clearDrawCameraState();
 FrameRenderCamera camera;
 core::setDrawCameraStateFromCamera(camera);
 Matrix4f outer;
 outer.translate(1.0f, 2.0f, 3.0f);
 core::setDrawPose(outer);
 {
  const core::ScopedDrawCameraState guard;
  Matrix4f inner;
  inner.translate(-7.0f, 0.0f, 0.0f);
  core::setDrawPose(inner);
  EXPECT_TRUE(nearMatrix(core::drawPose(), inner));
 }
 // Nested entity/item renders must hand the caller its pose back untouched.
 EXPECT_TRUE(nearMatrix(core::drawPose(), outer));
 // A camera publish is a pass boundary (GUI, hand): the world pose is gone.
 gui_proj::load(854.0f, 480.0f);
 EXPECT_TRUE(nearMatrix(core::drawPose(), Matrix4f::identityMatrix()));
}
TEST(DrawCameraState, ClearDrawCameraStateInvalidates) {
 core::clearDrawCameraState();
 ASSERT_FALSE(core::drawCameraStateValid());
 gui_proj::load(854.0f, 480.0f);
 ASSERT_TRUE(core::drawCameraStateValid());
 core::clearDrawCameraState();
 ASSERT_FALSE(core::drawCameraStateValid());
}
} // namespace net::minecraft::test
