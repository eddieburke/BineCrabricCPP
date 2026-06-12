#include "net/minecraft/client/render/entity/MinecartEntityRenderer.hpp"

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/client/gl/GL11.hpp"
#include "net/minecraft/client/render/block/BlockRenderManager.hpp"
#include "net/minecraft/client/render/entity/model/MinecartEntityModel.hpp"
#include "net/minecraft/entity/vehicle/MinecartEntity.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"

#include <cmath>

namespace net::minecraft::client::render::entity {

namespace {
constexpr int kChestBlockId = 54;
constexpr int kFurnaceBlockId = 61;
}

MinecartEntityRenderer::MinecartEntityRenderer()
    : blockRenderManager_()
{
    shadowRadius = 0.5f;
    model_ = new model::MinecartEntityModel();
}

MinecartEntityRenderer::~MinecartEntityRenderer()
{
    delete model_;
    model_ = nullptr;
}

void MinecartEntityRenderer::render(const net::minecraft::Entity& entity, double x, double y, double z, float yaw, float tickDelta)
{
    const auto* cart = dynamic_cast<const ::net::minecraft::entity::vehicle::MinecartEntity*>(&entity);
    if (cart == nullptr || model_ == nullptr) {
        return;
    }

    gl::GL11::glPushMatrix();
    const double interpX = cart->lastTickX + (cart->x - cart->lastTickX) * static_cast<double>(tickDelta);
    const double interpY = cart->lastTickY + (cart->y - cart->lastTickY) * static_cast<double>(tickDelta);
    const double interpZ = cart->lastTickZ + (cart->z - cart->lastTickZ) * static_cast<double>(tickDelta);
    constexpr double railOffset = 0.3;
    float renderPitch = cart->prevPitch + (cart->pitch - cart->prevPitch) * tickDelta;
    float renderYaw = yaw;

    if (const std::optional<Vec3d> railPos = cart->snapPositionToRail(interpX, interpY, interpZ)) {
        std::optional<Vec3d> frontPos = cart->snapPositionToRailWithOffset(interpX, interpY, interpZ, railOffset);
        std::optional<Vec3d> backPos = cart->snapPositionToRailWithOffset(interpX, interpY, interpZ, -railOffset);
        if (!frontPos.has_value()) {
            frontPos = railPos;
        }
        if (!backPos.has_value()) {
            backPos = railPos;
        }
        x += railPos->x - interpX;
        y += (frontPos->y + backPos->y) / 2.0 - interpY;
        z += railPos->z - interpZ;
        Vec3d direction {backPos->x - frontPos->x, backPos->y - frontPos->y, backPos->z - frontPos->z};
        const double directionLength = std::sqrt(direction.x * direction.x + direction.y * direction.y + direction.z * direction.z);
        if (directionLength != 0.0) {
            direction = direction.normalize();
            renderYaw = static_cast<float>(std::atan2(direction.z, direction.x) * 180.0 / 3.14159265358979323846);
            renderPitch = static_cast<float>(std::atan(direction.y) * 73.0);
        }
    }

    gl::GL11::glTranslatef(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z));
    gl::GL11::glRotatef(180.0f - renderYaw, 0.0f, 1.0f, 0.0f);
    gl::GL11::glRotatef(-renderPitch, 0.0f, 0.0f, 1.0f);

    float wobbleTicks = static_cast<float>(cart->damageWobbleTicks) - tickDelta;
    float wobbleStrength = cart->damageWobbleStrength - tickDelta;
    if (wobbleStrength < 0.0f) {
        wobbleStrength = 0.0f;
    }
    if (wobbleTicks > 0.0f) {
        gl::GL11::glRotatef(
            MathHelper::sin(wobbleTicks) * wobbleTicks * wobbleStrength / 10.0f * static_cast<float>(cart->damageWobbleSide),
            1.0f, 0.0f, 0.0f);
    }

    if (cart->type != 0) {
        bindTexture("/terrain.png");
        constexpr float cargoScale = 0.75f;
        gl::GL11::glScalef(cargoScale, cargoScale, cargoScale);
        gl::GL11::glTranslatef(0.0f, 0.3125f, 0.0f);
        gl::GL11::glRotatef(90.0f, 0.0f, 1.0f, 0.0f);
        const float brightness = cart->getBrightnessAtEyes(tickDelta);
        if (cart->type == 1) {
            if (net::minecraft::block::Block* chest = net::minecraft::block::Block::BLOCKS[kChestBlockId]) {
                blockRenderManager_.render(*chest, 0, brightness);
            }
        } else if (cart->type == 2) {
            if (net::minecraft::block::Block* furnace = net::minecraft::block::Block::BLOCKS[kFurnaceBlockId]) {
                blockRenderManager_.render(*furnace, 0, brightness);
            }
        }
        gl::GL11::glRotatef(-90.0f, 0.0f, 1.0f, 0.0f);
        gl::GL11::glTranslatef(0.0f, -0.3125f, 0.0f);
        gl::GL11::glScalef(1.0f / cargoScale, 1.0f / cargoScale, 1.0f / cargoScale);
    }

    bindTexture("/item/cart.png");
    gl::GL11::glScalef(-1.0f, -1.0f, 1.0f);
    model_->render(0.0f, 0.0f, -0.1f, 0.0f, 0.0f, 0.0625f);
    gl::GL11::glPopMatrix();
}

} // namespace net::minecraft::client::render::entity

#include "net/minecraft/client/entity/EntityClientRendererRegistration.hpp"
#include "net/minecraft/entity/vehicle/MinecartEntity.hpp"

namespace net::minecraft::entity::vehicle {

std::unique_ptr<::net::minecraft::client::render::entity::EntityRenderer> MinecartEntity::ClientRenderer::create()
{
    return std::make_unique<::net::minecraft::client::render::entity::MinecartEntityRenderer>();
}

} // namespace net::minecraft::entity::vehicle

namespace {

static ::net::minecraft::registry::RegisterEntityRenderer<net::minecraft::entity::vehicle::MinecartEntity> autoRendererReg;

} // namespace
