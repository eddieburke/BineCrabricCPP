#pragma once
#include "net/minecraft/client/render/atmosphere/CloudRenderer.hpp"
#include "net/minecraft/client/render/RenderTargetManager.hpp"
#include "net/minecraft/client/render/atmosphere/PrecipitationRenderer.hpp"
#include "net/minecraft/client/render/FrameRenderCamera.hpp"
#include "net/minecraft/client/render/item/HeldItemRenderer.hpp"
#include "net/minecraft/client/util/SmoothUtil.hpp"
#include "net/minecraft/entity/EntityTypes.hpp"
#include "net/minecraft/util/math/Types.hpp"
#include <cstdint>
#include <memory>
#include <optional>
namespace net::minecraft::client {
class Minecraft;
}
namespace net::minecraft::client::render {
class GameRenderer {
public:
  explicit GameRenderer(net::minecraft::client::Minecraft* client);
  void updateCamera();
  void updateTargetedEntity(float tickDelta);
  void onFrameUpdate(float tickDelta);
  void setupHudRender();
  void renderFrame(float tickDelta, std::int64_t timeNs = 0);
  void renderToCurrentTarget(float tickDelta, const FrameRenderCamera& camera, float fov, int viewportWidth,
                             int viewportHeight, bool renderCameraEntity);
  [[nodiscard]] RenderTargetManager& renderTargets() noexcept {
    return renderTargets_;
  }
  [[nodiscard]] item::HeldItemRenderer* heldItemRendererPtr() {
    return heldItemRenderer.get();
  }
  [[nodiscard]] const item::HeldItemRenderer* heldItemRendererPtr() const {
    return heldItemRenderer.get();
  }

private:
  float getFov(float tickDelta) const;
  void applyDamageTiltEffect(float tickDelta);
  void applyViewBobbing(float tickDelta);
  void applyCameraTransform(float tickDelta);
  void updateSkyAndFogColors(float tickDelta);
  void applyFog(int mode);
  void renderWorld(float tickDelta, float fov);
  void renderFirstPersonHand(float tickDelta);
  void renderRain();
  void throttleAndTimestamp(int fpsCap);
  net::minecraft::client::Minecraft* client = nullptr;
  atmosphere::CloudRenderer cloudRenderer{};
  atmosphere::PrecipitationRenderer precipitationRenderer{};
  std::unique_ptr<item::HeldItemRenderer> heldItemRenderer;
  int ticks = 0;
  net::minecraft::Entity* targetedEntity = nullptr;
  util::SmoothUtil cinematicCameraYawSmoother{};
  util::SmoothUtil cinematicCameraPitchSmoother{};
  util::SmoothUtil yawSmoother{};
  util::SmoothUtil pitchSmoother{};
  float thirdPersonDistance = 4.0f;
  float prevThirdPersonDistance = 4.0f;
  float thirdPersonYaw = 0.0f;
  float prevThirdPersonYaw = 0.0f;
  float thirdPersonPitch = 0.0f;
  float prevThirdPersonPitch = 0.0f;
  float cameraRoll = 0.0f;
  float prevCameraRoll = 0.0f;
  float cameraRollAmount = 0.0f;
  float prevCameraRollAmount = 0.0f;
  double zoom = 1.0;
  double zoomX = 0.0;
  double zoomY = 0.0;
  std::int64_t lastInactiveTime = 0;
  std::int64_t lastFrameTime = 0;
  JavaRandom random{};
  float lastViewBob = 0.0f;
  float viewBob = 0.0f;
  float fogRed = 0.0f;
  float fogGreen = 0.0f;
  float fogBlue = 0.0f;
  FrameRenderCamera frameCamera_{};
  RenderTargetManager renderTargets_{};
};
} // namespace net::minecraft::client::render
