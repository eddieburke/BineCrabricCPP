// Draw camera state (the single matrix source after the matrix-stack collapse).
// Pure CPU: exercises setDrawCameraState / setDrawCameraStateFromCamera /
// setDrawModelView / ScopedDrawCameraState / gui_proj ortho publishing.
#include <gtest/gtest.h>
#include <cmath>
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
TEST(DrawCameraState, FromCameraBuildsFullPerDrawBase) {
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
 core::setDrawCameraStateFromCamera(camera, 256.0f);
 ASSERT_TRUE(core::drawCameraStateValid());
 // Per-draw base = R * T(origin - eye): a camera-space point p is mapped as
 // R * p + R * (origin - eye) (the old stack top carried the same translation).
 // R here: col0=(0,1,0), col1=(0,0,1), col2=(-1,0,0); origin-eye=(-12,10,-15),
 // so R*p = (-pz, px, py) and R*(origin-eye) = (15, -12, 10).
 const Matrix4f& modelView = core::drawModelView();
 const float px = 4.0f;
 const float py = -2.0f;
 const float pz = 7.0f;
 float outX = 0.0f;
 float outY = 0.0f;
 float outZ = 0.0f;
 modelView.transformPoint(px, py, pz, outX, outY, outZ);
 EXPECT_NEAR(outX, -pz + 15.0f, 1e-4f);
 EXPECT_NEAR(outY, px - 12.0f, 1e-4f);
 EXPECT_NEAR(outZ, py + 10.0f, 1e-4f);
 // Projection matches the camera projection build.
 const Matrix4f& projection = core::drawProjection();
 EXPECT_NEAR(projection.m[0], 1.2f, 1e-5f);
 EXPECT_NEAR(projection.m[5], 1.1f, 1e-5f);
 EXPECT_NEAR(projection.m[11], -1.0f, 1e-5f);
 // The camera position published is the eye.
 const float* cameraPosition = core::drawCameraPosition();
 EXPECT_NEAR(cameraPosition[0], 112.0f, 1e-3f);
 EXPECT_NEAR(cameraPosition[1], 190.0f, 1e-3f);
 EXPECT_NEAR(cameraPosition[2], 315.0f, 1e-3f);
}
TEST(DrawCameraState, SetDrawModelViewComposesPerDrawPose) {
 core::clearDrawCameraState();
 gui_proj::load(854.0f, 480.0f);
 const Matrix4f base = core::drawModelView();
 const float baseZ = base.m[14];
 Matrix4f pose = base;
 pose.translate(10.0f, -20.0f, 0.0f);
 core::setDrawModelView(pose);
 const Matrix4f& published = core::drawModelView();
 EXPECT_NEAR(published.m[12], 10.0f, 1e-5f);
 EXPECT_NEAR(published.m[13], -20.0f, 1e-5f);
 EXPECT_NEAR(published.m[14], baseZ, 1e-5f);
 EXPECT_TRUE(core::drawCameraStateValid());
 // The projection is untouched by a per-draw pose publish.
 EXPECT_NEAR(core::drawProjection().m[0], 2.0f / 854.0f, 1e-5f);
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
  core::setDrawCameraStateFromCamera(camera, 256.0f);
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
TEST(DrawCameraState, ClearDrawCameraStateInvalidates) {
 core::clearDrawCameraState();
 ASSERT_FALSE(core::drawCameraStateValid());
 gui_proj::load(854.0f, 480.0f);
 ASSERT_TRUE(core::drawCameraStateValid());
 core::clearDrawCameraState();
 ASSERT_FALSE(core::drawCameraStateValid());
}
} // namespace net::minecraft::test
