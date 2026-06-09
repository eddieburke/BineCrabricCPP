#pragma once

#include "net/minecraft/client/render/entity/EntityRenderer.hpp"
#include "net/minecraft/client/render/entity/model/EntityModel.hpp"

namespace net::minecraft::client::render::entity {

class BoatEntityRenderer : public EntityRenderer {
public:
    BoatEntityRenderer();
    ~BoatEntityRenderer() override;

    void render(const net::minecraft::Entity& entity, double x, double y, double z, float yaw, float tickDelta) override;

private:
    model::EntityModel* model_ = nullptr;
};

} // namespace net::minecraft::client::render::entity
