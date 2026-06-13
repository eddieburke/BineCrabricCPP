#pragma once

#include "net/minecraft/client/render/item/HeldItemRenderer.hpp"
#include "net/minecraft/client/render/ViewDistance.hpp"
#include "net/minecraft/client/util/SmoothUtil.hpp"
#include "net/minecraft/entity/EntityTypes.hpp"
#include "net/minecraft/util/math/Types.hpp"

#include <array>
#include <cstdint>
#include <memory>
#include <optional>

namespace net::minecraft::client {
class Minecraft;
}

namespace net::minecraft::client::render {

class GameRenderer;

namespace pipeline {
class WorldRenderPass;
}

namespace platform {
void renderStereoFrame(GameRenderer& renderer, pipeline::WorldRenderPass& worldRenderPass, float tickDelta,
    std::int64_t timeNs);
}

// Faithful port of net.minecraft.client.render.GameRenderer (beta 1.7.3 MCP).
class GameRenderer {
public:
    friend class pipeline::WorldRenderPass;
    friend void platform::renderStereoFrame(GameRenderer&, pipeline::WorldRenderPass&, float, std::int64_t);

    explicit GameRenderer(net::minecraft::client::Minecraft* client);

    void updateCamera();
    void updateTargetedEntity(float tickDelta);
    void onFrameUpdate(float tickDelta);
    void setupHudRender();
    void renderFrame(float tickDelta, std::int64_t timeNs = 0);

    [[nodiscard]] item::HeldItemRenderer* heldItemRendererPtr() { return heldItemRenderer.get(); }
    [[nodiscard]] const item::HeldItemRenderer* heldItemRendererPtr() const { return heldItemRenderer.get(); }

private:
    float getFov(float tickDelta) const;
    void applyDamageTiltEffect(float tickDelta);
    void applyViewBobbing(float tickDelta);
    void applyCameraTransform(float tickDelta);
    void renderWorld(float tickDelta, int eye);
    void renderFirstPersonHand(float tickDelta, int eye);
    void renderRain();
    void throttleAndTimestamp(int fpsCap);

    net::minecraft::client::Minecraft* client = nullptr;
    ViewDistance viewDistance {};
    std::unique_ptr<item::HeldItemRenderer> heldItemRenderer;
    int ticks = 0;
    net::minecraft::Entity* targetedEntity = nullptr;
    util::SmoothUtil cinematicCameraYawSmoother {};
    util::SmoothUtil cinematicCameraPitchSmoother {};
    util::SmoothUtil yawSmoother {};
    util::SmoothUtil pitchSmoother {};
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
    JavaRandom random {};
    int rainSoundCounter = 0;
    float lastViewBob = 0.0f;
    float viewBob = 0.0f;
};

} // namespace net::minecraft::client::render
