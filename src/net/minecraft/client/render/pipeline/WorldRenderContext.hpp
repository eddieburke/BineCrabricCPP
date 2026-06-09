#pragma once

#include "net/minecraft/client/render/culling/FrustumCuller.hpp"
#include "net/minecraft/client/render/pipeline/PipelineTypes.hpp"

#include "net/minecraft/entity/EntityForward.hpp"

#include <cstdint>

namespace net::minecraft::client {
class Minecraft;
}

namespace net::minecraft::client::render {

class GameRenderer;
class Culler;
class WorldRenderer;

namespace atmosphere {
class AtmosphereRenderer;
}

namespace pipeline {

// Mutable per-eye state threaded through pipeline passes.
struct WorldRenderContext {
    GameRenderer& renderer;
    float tickDelta = 0.0f;
    std::int64_t timeNs = 0;
    int eye = 0;
    ColorMaskRestoreFn restoreColorMask {};
    bool clearColorBuffer = true;

    net::minecraft::client::Minecraft* client = nullptr;
    double zoom = 1.0;
    net::minecraft::entity::LivingEntity* camera = nullptr;
    WorldRenderer* worldRenderer = nullptr;
    atmosphere::AtmosphereRenderer* atmosphere = nullptr;
    double camX = 0.0;
    double camY = 0.0;
    double camZ = 0.0;
    int terrainTextureId = 0;

    FrustumCuller frustumCuller {};
    Culler* activeCuller = nullptr;
};

} // namespace pipeline
} // namespace net::minecraft::client::render
