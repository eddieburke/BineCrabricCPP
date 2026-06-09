#pragma once

#include "net/minecraft/client/render/entity/EntityRenderer.hpp"

#include <random>

namespace net::minecraft::entity::decoration::painting {
class PaintingEntity;
}

namespace net::minecraft::client::render::entity {

class PaintingEntityRenderer : public EntityRenderer {
public:
    void render(const net::minecraft::Entity& entity, double x, double y, double z, float yaw, float tickDelta) override;

private:
    void renderPainting(const ::net::minecraft::entity::decoration::painting::PaintingEntity& painting, int width, int height, int u, int v);
    void applyBrightness(const ::net::minecraft::entity::decoration::painting::PaintingEntity& painting, float u, float v);

    std::mt19937_64 random_ {187ULL};
};

} // namespace net::minecraft::client::render::entity
