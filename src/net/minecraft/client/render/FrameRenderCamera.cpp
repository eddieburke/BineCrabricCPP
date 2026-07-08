#include "net/minecraft/client/render/FrameRenderCamera.hpp"
#include "net/minecraft/entity/LivingEntity.hpp"
#include "net/minecraft/mod/events/RenderEvents.hpp"
namespace net::minecraft::client::render {
RenderCameraState& RenderCameraState::instance() noexcept {
  static RenderCameraState state;
  return state;
}
void RenderCameraState::setFrame(FrameRenderCamera camera) noexcept {
  frame_ = camera;
}
void RenderCameraState::clearFrame() noexcept {
  frame_ = {};
}
void populateCameraSetupDefaults(mod::CameraSetupEvent& event, const net::minecraft::entity::LivingEntity& camera,
                                 float tickDelta, float rollDegrees) {
  event.camera = const_cast<net::minecraft::entity::LivingEntity*>(&camera);
  event.tickDelta = tickDelta;
  const double eyeOffset = static_cast<double>(camera.standingEyeHeight - 1.62f);
  event.x = camera.lastTickX + (camera.x - camera.lastTickX) * static_cast<double>(tickDelta);
  event.y = camera.lastTickY + (camera.y - camera.lastTickY) * static_cast<double>(tickDelta) - eyeOffset;
  event.z = camera.lastTickZ + (camera.z - camera.lastTickZ) * static_cast<double>(tickDelta);
  event.yaw = camera.prevYaw + (camera.yaw - camera.prevYaw) * tickDelta;
  event.pitch = camera.prevPitch + (camera.pitch - camera.prevPitch) * tickDelta;
  event.roll = rollDegrees;
  event.customView = false;
  event.hideFirstPersonHand = false;
}
FrameRenderCamera frameCameraFromSetup(const mod::CameraSetupEvent& event) noexcept {
  FrameRenderCamera frame{};
  frame.x = event.x;
  frame.y = event.y;
  frame.z = event.z;
  frame.yaw = event.yaw;
  frame.pitch = event.pitch;
  frame.roll = event.roll;
  frame.customView = event.customView;
  frame.hideFirstPersonHand = event.hideFirstPersonHand;
  return frame;
}
} // namespace net::minecraft::client::render
