#pragma once

#include "net/minecraft/entity/EntityForward.hpp"
#include "net/minecraft/util/math/Types.hpp"

namespace net::minecraft::client::render {

class Culler;
class WorldRenderer;

namespace internal {

class WorldRendererCore {
public:
    static void renderLastChunks(WorldRenderer& worldRenderer, int layer, double tickDelta);
    static void render(WorldRenderer& worldRenderer, const entity::Entity& camera, int layer, float tickDelta);
    static int render(WorldRenderer& worldRenderer, entity::LivingEntity& camera, int layer, double tickDelta);
    static bool compileChunks(WorldRenderer& worldRenderer, entity::LivingEntity& camera, bool force);
    static void renderEntities(WorldRenderer& worldRenderer, const Vec3d& cameraPos, Culler* culler, float tickDelta);

private:
    static int renderChunks(WorldRenderer& worldRenderer, int layer, double tickDelta);
};

} // namespace internal
} // namespace net::minecraft::client::render
