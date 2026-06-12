#include "net/minecraft/client/render/entity/ChickenEntityRenderer.hpp"

#include "net/minecraft/client/render/entity/EntityRendererCasts.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"

namespace net::minecraft::client::render::entity {

float ChickenEntityRenderer::getHeadBob(const net::minecraft::LivingEntity& entity, float tickDelta) const
{
    const auto* chicken = dynamic_cast<const casts::ChickenEntity*>(&entity);
    if (chicken == nullptr) {
        return LivingEntityRenderer::getHeadBob(entity, tickDelta);
    }
    const float flap = chicken->prevFlapProgress + (chicken->flapProgress - chicken->prevFlapProgress) * tickDelta;
    const float deviation =
        chicken->prevMaxWingDeviation + (chicken->maxWingDeviation - chicken->prevMaxWingDeviation) * tickDelta;
    return (MathHelper::sin(flap) + 1.0f) * deviation;
}

} // namespace net::minecraft::client::render::entity

#include "net/minecraft/client/entity/EntityClientRendererRegistration.hpp"
#include "net/minecraft/client/render/entity/model/ChickenEntityModel.hpp"
#include "net/minecraft/entity/passive/ChickenEntity.hpp"

namespace net::minecraft::entity::passive {

std::unique_ptr<::net::minecraft::client::render::entity::EntityRenderer> ChickenEntity::ClientRenderer::create()
{
    using namespace ::net::minecraft::client::render::entity;
    using namespace ::net::minecraft::client::render::entity::model;
    return std::make_unique<ChickenEntityRenderer>(new ChickenEntityModel(), 0.3f);
}

} // namespace net::minecraft::entity::passive

namespace {

static ::net::minecraft::registry::RegisterEntityRenderer<net::minecraft::entity::passive::ChickenEntity> autoRendererReg;

} // namespace
