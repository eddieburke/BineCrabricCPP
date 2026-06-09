#include "net/minecraft/client/render/entity/EntityRenderDispatcher.hpp"

#include "net/minecraft/client/render/item/HeldItemRenderer.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/client/gl/GL11.hpp"
#include "net/minecraft/client/render/entity/ArrowEntityRenderer.hpp"
#include "net/minecraft/client/render/entity/BoatEntityRenderer.hpp"
#include "net/minecraft/client/render/entity/BoxEntityRenderer.hpp"
#include "net/minecraft/client/render/entity/ChickenEntityRenderer.hpp"
#include "net/minecraft/client/render/entity/CowEntityRenderer.hpp"
#include "net/minecraft/client/render/entity/CreeperEntityRenderer.hpp"
#include "net/minecraft/client/render/entity/EntityRenderer.hpp"
#include "net/minecraft/client/render/entity/FallingBlockEntityRenderer.hpp"
#include "net/minecraft/client/render/entity/FireballEntityRenderer.hpp"
#include "net/minecraft/client/render/entity/FishingBobberEntityRenderer.hpp"
#include "net/minecraft/client/render/entity/GhastEntityRenderer.hpp"
#include "net/minecraft/client/render/entity/GiantEntityRenderer.hpp"
#include "net/minecraft/client/render/entity/ItemEntityRenderer.hpp"
#include "net/minecraft/client/render/entity/LightningEntityRenderer.hpp"
#include "net/minecraft/client/render/entity/LivingEntityRenderer.hpp"
#include "net/minecraft/client/render/entity/MinecartEntityRenderer.hpp"
#include "net/minecraft/client/render/entity/PaintingEntityRenderer.hpp"
#include "net/minecraft/client/render/entity/PigEntityRenderer.hpp"
#include "net/minecraft/client/render/entity/PlayerEntityRenderer.hpp"
#include "net/minecraft/client/render/entity/ProjectileEntityRenderer.hpp"
#include "net/minecraft/client/render/entity/SheepEntityRenderer.hpp"
#include "net/minecraft/client/render/entity/SlimeEntityRenderer.hpp"
#include "net/minecraft/client/render/entity/SpiderEntityRenderer.hpp"
#include "net/minecraft/client/render/entity/SquidEntityRenderer.hpp"
#include "net/minecraft/client/render/entity/TntEntityRenderer.hpp"
#include "net/minecraft/client/render/entity/UndeadEntityRenderer.hpp"
#include "net/minecraft/client/render/entity/WolfEntityRenderer.hpp"
#include "net/minecraft/client/render/entity/model/BipedEntityModel.hpp"
#include "net/minecraft/client/render/entity/model/ChickenEntityModel.hpp"
#include "net/minecraft/client/render/entity/model/CowEntityModel.hpp"
#include "net/minecraft/client/render/entity/model/PigEntityModel.hpp"
#include "net/minecraft/client/render/entity/model/SheepEntityModel.hpp"
#include "net/minecraft/client/render/entity/model/SheepWoolEntityModel.hpp"
#include "net/minecraft/client/render/entity/model/SkeletonEntityModel.hpp"
#include "net/minecraft/client/render/entity/model/SlimeEntityModel.hpp"
#include "net/minecraft/client/render/entity/model/SquidEntityModel.hpp"
#include "net/minecraft/client/render/entity/model/WolfEntityModel.hpp"
#include "net/minecraft/client/render/entity/model/ZombieEntityModel.hpp"
#include "net/minecraft/entity/Entity.hpp"
#include "net/minecraft/entity/FallingBlockEntity.hpp"
#include "net/minecraft/entity/ItemEntity.hpp"
#include "net/minecraft/entity/LightningEntity.hpp"
#include "net/minecraft/entity/TntEntity.hpp"
#include "net/minecraft/entity/decoration/painting/PaintingEntity.hpp"
#include "net/minecraft/entity/mob/CreeperEntity.hpp"
#include "net/minecraft/entity/mob/GhastEntity.hpp"
#include "net/minecraft/entity/mob/GiantEntity.hpp"
#include "net/minecraft/entity/mob/SkeletonEntity.hpp"
#include "net/minecraft/entity/mob/SlimeEntity.hpp"
#include "net/minecraft/entity/mob/SpiderEntity.hpp"
#include "net/minecraft/entity/mob/ZombieEntity.hpp"
#include "net/minecraft/entity/passive/ChickenEntity.hpp"
#include "net/minecraft/entity/passive/CowEntity.hpp"
#include "net/minecraft/entity/passive/PigEntity.hpp"
#include "net/minecraft/entity/passive/SheepEntity.hpp"
#include "net/minecraft/entity/passive/SquidEntity.hpp"
#include "net/minecraft/entity/passive/WolfEntity.hpp"
#include "net/minecraft/entity/projectile/ArrowEntity.hpp"
#include "net/minecraft/entity/projectile/FireballEntity.hpp"
#include "net/minecraft/entity/projectile/FishingBobberEntity.hpp"
#include "net/minecraft/entity/projectile/thrown/EggEntity.hpp"
#include "net/minecraft/entity/projectile/thrown/SnowballEntity.hpp"
#include "net/minecraft/entity/vehicle/BoatEntity.hpp"
#include "net/minecraft/entity/vehicle/MinecartEntity.hpp"
#include "net/minecraft/entity/LivingEntity.hpp"
#include "net/minecraft/entity/player/ClientPlayerEntity.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"

namespace net::minecraft::client::render::entity {

namespace {
constexpr int kSnowballTexture = 14; // Item.SNOWBALL texture index (14, 0)
constexpr int kEggTexture = 12;      // Item.EGG texture index (12, 0)
}

EntityRenderDispatcher::EntityRenderDispatcher()
    : options_(defaultOptions_)
    , heldItemRenderer_(std::make_unique<net::minecraft::client::render::item::HeldItemRenderer>())
{
    registerRenderer<::net::minecraft::entity::mob::SpiderEntity>(std::make_unique<SpiderEntityRenderer>());
    registerRenderer<::net::minecraft::entity::passive::PigEntity>(std::make_unique<PigEntityRenderer>(
        new model::PigEntityModel(), new model::PigEntityModel(0.5f), 0.7f));
    registerRenderer<::net::minecraft::entity::passive::SheepEntity>(std::make_unique<SheepEntityRenderer>(
        new model::SheepEntityModel(), new model::SheepWoolEntityModel(), 0.7f));
    registerRenderer<::net::minecraft::entity::passive::CowEntity>(
        std::make_unique<CowEntityRenderer>(new model::CowEntityModel(), 0.7f));
    registerRenderer<::net::minecraft::entity::passive::WolfEntity>(
        std::make_unique<WolfEntityRenderer>(new model::WolfEntityModel(), 0.5f));
    registerRenderer<::net::minecraft::entity::passive::ChickenEntity>(
        std::make_unique<ChickenEntityRenderer>(new model::ChickenEntityModel(), 0.3f));
    registerRenderer<::net::minecraft::entity::mob::CreeperEntity>(std::make_unique<CreeperEntityRenderer>());
    registerRenderer<::net::minecraft::entity::mob::SkeletonEntity>(
        std::make_unique<UndeadEntityRenderer>(new model::SkeletonEntityModel(), 0.5f));
    registerRenderer<::net::minecraft::entity::mob::ZombieEntity>(
        std::make_unique<UndeadEntityRenderer>(new model::ZombieEntityModel(), 0.5f));
    registerRenderer<::net::minecraft::entity::mob::SlimeEntity>(std::make_unique<SlimeEntityRenderer>(
        new model::SlimeEntityModel(16), new model::SlimeEntityModel(0), 0.25f));
    registerRenderer<net::minecraft::PlayerEntity>(std::make_unique<PlayerEntityRenderer>());
    registerRenderer<::net::minecraft::entity::mob::GiantEntity>(
        std::make_unique<GiantEntityRenderer>(new model::ZombieEntityModel(), 0.5f, 6.0f));
    registerRenderer<::net::minecraft::entity::mob::GhastEntity>(std::make_unique<GhastEntityRenderer>());
    registerRenderer<::net::minecraft::entity::passive::SquidEntity>(
        std::make_unique<SquidEntityRenderer>(new model::SquidEntityModel(), 0.7f));
    registerRenderer<net::minecraft::LivingEntity>(
        std::make_unique<LivingEntityRenderer>(new model::BipedEntityModel(), 0.5f));
    registerRenderer<net::minecraft::Entity>(std::make_unique<BoxEntityRenderer>());
    registerRenderer<::net::minecraft::entity::decoration::painting::PaintingEntity>(std::make_unique<PaintingEntityRenderer>());
    registerRenderer<::net::minecraft::entity::projectile::ArrowEntity>(std::make_unique<ArrowEntityRenderer>());
    registerRenderer<::net::minecraft::entity::projectile::thrown::SnowballEntity>(
        std::make_unique<ProjectileEntityRenderer>(kSnowballTexture));
    registerRenderer<::net::minecraft::entity::projectile::thrown::EggEntity>(std::make_unique<ProjectileEntityRenderer>(kEggTexture));
    registerRenderer<::net::minecraft::entity::projectile::FireballEntity>(std::make_unique<FireballEntityRenderer>());
    registerRenderer<::net::minecraft::ItemEntity>(std::make_unique<ItemEntityRenderer>());
    registerRenderer<net::minecraft::TntEntity>(std::make_unique<TntEntityRenderer>());
    registerRenderer<net::minecraft::FallingBlockEntity>(std::make_unique<FallingBlockEntityRenderer>());
    registerRenderer<::net::minecraft::entity::vehicle::MinecartEntity>(std::make_unique<MinecartEntityRenderer>());
    registerRenderer<::net::minecraft::entity::vehicle::BoatEntity>(std::make_unique<BoatEntityRenderer>());
    registerRenderer<::net::minecraft::entity::projectile::FishingBobberEntity>(std::make_unique<FishingBobberEntityRenderer>());
    registerRenderer<net::minecraft::LightningEntity>(std::make_unique<LightningEntityRenderer>());
}

EntityRenderDispatcher::~EntityRenderDispatcher() = default;

void EntityRenderDispatcher::setHeldItemRenderer(
    std::unique_ptr<net::minecraft::client::render::item::HeldItemRenderer> renderer)
{
    heldItemRenderer_ = std::move(renderer);
}

void EntityRenderDispatcher::registerRenderer(std::type_index key, std::unique_ptr<EntityRenderer> renderer)
{
    renderer->setDispatcher(this);
    renderers_[key] = std::move(renderer);
}

EntityRenderer* EntityRenderDispatcher::get(const std::type_info& entityType)
{
    const std::type_index key(entityType);
    const auto it = renderers_.find(key);
    if (it != renderers_.end()) {
        return it->second.get();
    }
    if (entityType == typeid(net::minecraft::Entity)) {
        const auto fallback = renderers_.find(std::type_index(typeid(net::minecraft::Entity)));
        return fallback != renderers_.end() ? fallback->second.get() : nullptr;
    }
    if (entityType == typeid(net::minecraft::entity::player::ClientPlayerEntity)) {
        return get(typeid(net::minecraft::PlayerEntity));
    }
    if (entityType == typeid(net::minecraft::Entity)) {
        return renderers_.at(std::type_index(typeid(net::minecraft::Entity))).get();
    }
    if (entityType != typeid(net::minecraft::LivingEntity)) {
        return get(typeid(net::minecraft::LivingEntity));
    }
    return get(typeid(net::minecraft::Entity));
}

EntityRenderer* EntityRenderDispatcher::get(const net::minecraft::Entity& entity)
{
    return get(typeid(entity));
}

void EntityRenderDispatcher::init(net::minecraft::World* world, net::minecraft::client::texture::TextureManager* textureManager,
    net::minecraft::client::font::TextRenderer* textRenderer, net::minecraft::LivingEntity* cameraEntity,
    net::minecraft::client::option::GameOptions* options, float tickDelta)
{
    world_ = world;
    textureManager_ = textureManager;
    textRenderer_ = textRenderer;
    cameraEntity_ = cameraEntity;
    if (options != nullptr) {
        defaultOptions_ = *options;
    }
    if (cameraEntity != nullptr && cameraEntity->isSleeping() && world != nullptr) {
        const int blockId = world->getBlockId(MathHelper::floor(cameraEntity->x), MathHelper::floor(cameraEntity->y),
            MathHelper::floor(cameraEntity->z));
        if (blockId == 26) { // Block.BED
            const int meta = world->getBlockMeta(MathHelper::floor(cameraEntity->x), MathHelper::floor(cameraEntity->y),
                MathHelper::floor(cameraEntity->z));
            const int bedDir = meta & 3;
            yaw_ = static_cast<float>(bedDir * 90 + 180);
            pitch_ = 0.0f;
        }
    } else if (cameraEntity != nullptr) {
        yaw_ = cameraEntity->prevYaw + (cameraEntity->yaw - cameraEntity->prevYaw) * tickDelta;
        pitch_ = cameraEntity->prevPitch + (cameraEntity->pitch - cameraEntity->prevPitch) * tickDelta;
    }
    if (cameraEntity != nullptr) {
        x_ = cameraEntity->lastTickX + (cameraEntity->x - cameraEntity->lastTickX) * static_cast<double>(tickDelta);
        y_ = cameraEntity->lastTickY + (cameraEntity->y - cameraEntity->lastTickY) * static_cast<double>(tickDelta);
        z_ = cameraEntity->lastTickZ + (cameraEntity->z - cameraEntity->lastTickZ) * static_cast<double>(tickDelta);
    }
}

void EntityRenderDispatcher::setWorld(net::minecraft::World* world)
{
    world_ = world;
}

void EntityRenderDispatcher::render(const net::minecraft::Entity& entity, float tickDelta)
{
    const double x = entity.lastTickX + (entity.x - entity.lastTickX) * static_cast<double>(tickDelta) - offsetX;
    const double y = entity.lastTickY + (entity.y - entity.lastTickY) * static_cast<double>(tickDelta) - offsetY;
    const double z = entity.lastTickZ + (entity.z - entity.lastTickZ) * static_cast<double>(tickDelta) - offsetZ;
    const float yaw = entity.prevYaw + (entity.yaw - entity.prevYaw) * tickDelta;
    const float brightness = entity.getBrightnessAtEyes(tickDelta);
    gl::GL11::glColor3f(brightness, brightness, brightness);
    render(entity, x, y, z, yaw, tickDelta);
}

void EntityRenderDispatcher::render(const net::minecraft::Entity& entity, double x, double y, double z, float yaw,
    float tickDelta)
{
    if (EntityRenderer* renderer = get(entity); renderer != nullptr) {
        renderer->render(entity, x, y, z, yaw, tickDelta);
        renderer->postRender(entity, x, y, z, yaw, tickDelta);
    }
}

double EntityRenderDispatcher::squaredDistanceTo(double x, double y, double z) const
{
    const double dx = x - x_;
    const double dy = y - y_;
    const double dz = z - z_;
    return dx * dx + dy * dy + dz * dz;
}

EntityRenderDispatcher& EntityRenderDispatcher::instance()
{
    static EntityRenderDispatcher dispatcher;
    return dispatcher;
}

} // namespace net::minecraft::client::render::entity
