#pragma once

#include "net/minecraft/client/render/entity/LivingEntityRenderer.hpp"
#include "net/minecraft/client/render/entity/model/EntityModel.hpp"

namespace net::minecraft::client::render::entity {

class CowEntityRenderer : public LivingEntityRenderer {
public:
    CowEntityRenderer(model::EntityModel* model, float shadowRadius);
};

} // namespace net::minecraft::client::render::entity
