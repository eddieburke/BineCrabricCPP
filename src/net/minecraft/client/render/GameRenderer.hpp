#pragma once
#include <cstdint>
#include <memory>
#include <optional>
#include "net/minecraft/client/gl/GlConstants.hpp"
#include "net/minecraft/client/option/RenderSettings.hpp"
#include "net/minecraft/client/render/FrameRenderCamera.hpp"
#include "net/minecraft/client/render/GuiProjection.hpp"
#include "net/minecraft/client/render/ShadowMapPass.hpp"
#include "net/minecraft/client/render/atmosphere/CloudRenderer.hpp"
#include "net/minecraft/client/render/atmosphere/PrecipitationRenderer.hpp"
#include "net/minecraft/client/render/item/HeldItemRenderer.hpp"
#include "net/minecraft/client/render/shaderpack/ShaderUniforms.hpp"
#include "net/minecraft/client/util/SmoothUtil.hpp"
#include "net/minecraft/entity/EntityTypes.hpp"
#include "net/minecraft/mod/runtime/LuaDirectHooks.hpp"
#include "net/minecraft/util/math/Types.hpp"
namespace net::minecraft::client {
class Minecraft;
}
namespace net::minecraft::client::render::shaderpack {
class ShaderPackManager;
struct ShaderPackDefinition;
} // namespace net::minecraft::client::render::shaderpack
namespace net::minecraft::client::render {
class GameRenderer {
 friend shadowmap::ShadowMapResult shadowmap::update(shadowmap::ShadowMapState&,
                                                     GameRenderer&,
                                                     float,
                                                     const FrameRenderCamera&,
                                                     float,
                                                     const shaderpack::ShaderPackDefinition*);

 public:
 explicit GameRenderer(net::minecraft::client::Minecraft* client);
 ~GameRenderer();
 void updateCamera();
 void updateTargetedEntity(float tickDelta);
 void onFrameUpdate(float tickDelta);
 [[nodiscard]] item::HeldItemRenderer* heldItemRendererPtr() {
  return heldItemRenderer.get();
 }
 [[nodiscard]] const item::HeldItemRenderer* heldItemRendererPtr() const {
  return heldItemRenderer.get();
 }
 [[nodiscard]] shaderpack::ShaderPackManager* shaderPacks() {
  return shaderPacks_.get();
 }
 [[nodiscard]] const option::RenderSettings& frameSettings() const noexcept {
  return frameSettings_;
 }

 private:
 float getFov(float tickDelta) const;
 void applyDamageTiltEffect(float tickDelta);
 void applyViewBobbing(float tickDelta);
 void applyCameraTransform(float tickDelta);
 void renderWorld(float tickDelta, float fov);
 void renderFrame(float tickDelta);
 void renderToCurrentTarget(float tickDelta,
                            const FrameRenderCamera& camera,
                            float fov,
                            int viewportWidth,
                            int viewportHeight,
                            bool renderCameraEntity,
                            bool captureWorldDepth = false);
 bool renderWorldToFbo(unsigned int fbo,
                       int width,
                       int height,
                       float tickDelta,
                       const FrameRenderCamera& camera,
                       float fov,
                       FrameRenderCamera* outCamera = nullptr);
 bool beginSceneCapture();
 void resolveSceneCapture();
 void renderFirstPersonHand(float tickDelta);
 void renderRain();
 [[nodiscard]] shaderpack::FrameUniformSet buildFrameUniforms(float tickDelta,
                                                              float farPlane,
                                                              bool shadowAvailable) const;
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
 JavaRandom random{};
 float lastViewBob = 0.0f;
 float viewBob = 0.0f;
 option::RenderSettings frameSettings_{};
 FrameRenderCamera frameCamera_{};
 shadowmap::ShadowMapState shadowState_{};
 shadowmap::ShadowMapResult frameShadow_{};
 std::unique_ptr<shaderpack::ShaderPackManager> shaderPacks_;
};
} // namespace net::minecraft::client::render
