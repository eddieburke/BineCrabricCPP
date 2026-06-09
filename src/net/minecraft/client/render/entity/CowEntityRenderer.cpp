#include "net/minecraft/client/render/entity/CowEntityRenderer.hpp"

namespace net::minecraft::client::render::entity {

CowEntityRenderer::CowEntityRenderer(model::EntityModel* model, float shadowRadius)
    : LivingEntityRenderer(model, shadowRadius)
{
}

} // namespace net::minecraft::client::render::entity
