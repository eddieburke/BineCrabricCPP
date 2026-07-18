#pragma once
namespace net::minecraft::entity {
class LivingEntity;
}
namespace net::minecraft::mod {
struct CameraSetupEvent;
}
namespace net::minecraft::client::render {
struct FrameRenderCamera {
  double x = 0.0;
  double y = 0.0;
  double z = 0.0;
  // Actual GL eye position (includes the fixed first-person view offset and
  // view bobbing), unlike x/y/z which is the parametric camera position.
  // Shaders that reconstruct world positions from the modelview inverse must
  // use this origin or per-fragment positions shift as the view rotates.
  double eyeX = 0.0;
  double eyeY = 0.0;
  double eyeZ = 0.0;
  float viewRightX = 1.0f;
  float viewRightY = 0.0f;
  float viewRightZ = 0.0f;
  float viewUpX = 0.0f;
  float viewUpY = 1.0f;
  float viewUpZ = 0.0f;
  float viewForwardX = 0.0f;
  float viewForwardY = 0.0f;
  float viewForwardZ = 1.0f;
  float projectionX = 1.0f;
  float projectionY = 1.0f;
  float yaw = 0.0f;
  float pitch = 0.0f;
  float roll = 0.0f;
  bool customView = false;
  bool hideFirstPersonHand = false;
  bool orthographic = false;
  float orthoHalfWidth = 1.0f;
  float orthoHalfHeight = 1.0f;
  float orthoNear = -1.0f;
  float orthoFar = 1.0f;
  float perspectiveNear = 0.05f;
  float perspectiveFar = 0.0f;
  bool shadowPass = false;
  bool shadowEntities = true;
};
class RenderCameraState {
public:
  static RenderCameraState& instance() noexcept;
  void setFrame(FrameRenderCamera camera) noexcept;
  [[nodiscard]] const FrameRenderCamera& frame() const noexcept {
    return frame_;
  }
  void clearFrame() noexcept;

private:
  FrameRenderCamera frame_{};
};
void populateCameraSetupDefaults(mod::CameraSetupEvent& event,
                                 const net::minecraft::entity::LivingEntity& camera,
                                 float tickDelta,
                                 float rollDegrees);
[[nodiscard]] FrameRenderCamera frameCameraFromSetup(const mod::CameraSetupEvent& event) noexcept;
} // namespace net::minecraft::client::render
