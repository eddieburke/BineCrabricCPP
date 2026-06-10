// Vanilla entity renderer registration — mirrors Java EntityRenderDispatcher ctor entries.
// Mods: call addPendingRenderer from a separate client TU.

#include "net/minecraft/client/render/entity/ArrowEntityRenderer.hpp"
#include "net/minecraft/client/render/entity/BoatEntityRenderer.hpp"
#include "net/minecraft/client/render/entity/ChickenEntityRenderer.hpp"
#include "net/minecraft/client/render/entity/CowEntityRenderer.hpp"
#include "net/minecraft/client/render/entity/CreeperEntityRenderer.hpp"
#include "net/minecraft/client/render/entity/EntityRenderDispatcher.hpp"
#include "net/minecraft/client/render/entity/EntityRenderer.hpp"
#include "net/minecraft/client/render/entity/FallingBlockEntityRenderer.hpp"
#include "net/minecraft/client/render/entity/FireballEntityRenderer.hpp"
#include "net/minecraft/client/render/entity/FishingBobberEntityRenderer.hpp"
#include "net/minecraft/client/render/entity/GhastEntityRenderer.hpp"
#include "net/minecraft/client/render/entity/GiantEntityRenderer.hpp"
#include "net/minecraft/client/render/entity/ItemEntityRenderer.hpp"
#include "net/minecraft/client/render/entity/LightningEntityRenderer.hpp"
#include "net/minecraft/client/render/entity/MinecartEntityRenderer.hpp"
#include "net/minecraft/client/render/entity/PaintingEntityRenderer.hpp"
#include "net/minecraft/client/render/entity/PigEntityRenderer.hpp"
#include "net/minecraft/client/render/entity/ProjectileEntityRenderer.hpp"
#include "net/minecraft/client/render/entity/SheepEntityRenderer.hpp"
#include "net/minecraft/client/render/entity/SlimeEntityRenderer.hpp"
#include "net/minecraft/client/render/entity/SpiderEntityRenderer.hpp"
#include "net/minecraft/client/render/entity/SquidEntityRenderer.hpp"
#include "net/minecraft/client/render/entity/TntEntityRenderer.hpp"
#include "net/minecraft/client/render/entity/UndeadEntityRenderer.hpp"
#include "net/minecraft/client/render/entity/WolfEntityRenderer.hpp"
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
#include "net/minecraft/entity/FallingBlockEntity.hpp"
#include "net/minecraft/entity/ItemEntity.hpp"
#include "net/minecraft/entity/LightningEntity.hpp"
#include "net/minecraft/entity/TntEntity.hpp"
#include "net/minecraft/entity/decoration/painting/PaintingEntity.hpp"
#include "net/minecraft/entity/mob/CreeperEntity.hpp"
#include "net/minecraft/entity/mob/GhastEntity.hpp"
#include "net/minecraft/entity/mob/GiantEntity.hpp"
#include "net/minecraft/entity/mob/SlimeEntity.hpp"
#include "net/minecraft/entity/mob/SpiderEntity.hpp"
#include "net/minecraft/entity/mob/SkeletonEntity.hpp"
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

#include <functional>
#include <memory>
#include <typeindex>

namespace {

using net::minecraft::client::render::entity::EntityRenderDispatcher;
using net::minecraft::client::render::entity::EntityRenderer;

template<typename EntityType>
void registerRenderer(std::function<std::unique_ptr<EntityRenderer>()> factory)
{
    EntityRenderDispatcher::addPendingRenderer(std::type_index(typeid(EntityType)), std::move(factory));
}

struct VanillaEntityRendererRegistrations {
    VanillaEntityRendererRegistrations()
    {
        using namespace net::minecraft::client::render::entity;
        using namespace net::minecraft::client::render::entity::model;
        using net::minecraft::entity::FallingBlockEntity;
        using net::minecraft::entity::ItemEntity;
        using net::minecraft::entity::LightningEntity;
        using net::minecraft::entity::TntEntity;
        using net::minecraft::entity::decoration::painting::PaintingEntity;
        using net::minecraft::entity::mob::CreeperEntity;
        using net::minecraft::entity::mob::GhastEntity;
        using net::minecraft::entity::mob::GiantEntity;
        using net::minecraft::entity::mob::SkeletonEntity;
        using net::minecraft::entity::mob::SlimeEntity;
        using net::minecraft::entity::mob::SpiderEntity;
        using net::minecraft::entity::mob::ZombieEntity;
        using net::minecraft::entity::passive::ChickenEntity;
        using net::minecraft::entity::passive::CowEntity;
        using net::minecraft::entity::passive::PigEntity;
        using net::minecraft::entity::passive::SheepEntity;
        using net::minecraft::entity::passive::SquidEntity;
        using net::minecraft::entity::passive::WolfEntity;
        using net::minecraft::entity::projectile::ArrowEntity;
        using net::minecraft::entity::projectile::FireballEntity;
        using net::minecraft::entity::projectile::FishingBobberEntity;
        using net::minecraft::entity::projectile::thrown::EggEntity;
        using net::minecraft::entity::projectile::thrown::SnowballEntity;
        using net::minecraft::entity::vehicle::BoatEntity;
        using net::minecraft::entity::vehicle::MinecartEntity;

        registerRenderer<CreeperEntity>([] { return std::make_unique<CreeperEntityRenderer>(); });
        registerRenderer<SkeletonEntity>([] {
            return std::make_unique<UndeadEntityRenderer>(new SkeletonEntityModel(), 0.5f);
        });
        registerRenderer<ZombieEntity>([] {
            return std::make_unique<UndeadEntityRenderer>(new ZombieEntityModel(), 0.5f);
        });
        registerRenderer<SpiderEntity>([] { return std::make_unique<SpiderEntityRenderer>(); });
        registerRenderer<GhastEntity>([] { return std::make_unique<GhastEntityRenderer>(); });
        registerRenderer<GiantEntity>([] {
            return std::make_unique<GiantEntityRenderer>(new ZombieEntityModel(), 0.5f, 6.0f);
        });
        registerRenderer<SlimeEntity>([] {
            return std::make_unique<SlimeEntityRenderer>(new SlimeEntityModel(16), new SlimeEntityModel(0), 0.25f);
        });
        registerRenderer<PigEntity>([] {
            return std::make_unique<PigEntityRenderer>(new PigEntityModel(), new PigEntityModel(0.5f), 0.7f);
        });
        registerRenderer<SheepEntity>([] {
            return std::make_unique<SheepEntityRenderer>(new SheepEntityModel(), new SheepWoolEntityModel(), 0.7f);
        });
        registerRenderer<CowEntity>([] {
            return std::make_unique<CowEntityRenderer>(new CowEntityModel(), 0.7f);
        });
        registerRenderer<ChickenEntity>([] {
            return std::make_unique<ChickenEntityRenderer>(new ChickenEntityModel(), 0.3f);
        });
        registerRenderer<SquidEntity>([] {
            return std::make_unique<SquidEntityRenderer>(new SquidEntityModel(), 0.7f);
        });
        registerRenderer<WolfEntity>([] {
            return std::make_unique<WolfEntityRenderer>(new WolfEntityModel(), 0.5f);
        });
        registerRenderer<TntEntity>([] { return std::make_unique<TntEntityRenderer>(); });
        registerRenderer<FallingBlockEntity>([] { return std::make_unique<FallingBlockEntityRenderer>(); });
        registerRenderer<ItemEntity>([] { return std::make_unique<ItemEntityRenderer>(); });
        registerRenderer<LightningEntity>([] { return std::make_unique<LightningEntityRenderer>(); });
        registerRenderer<ArrowEntity>([] { return std::make_unique<ArrowEntityRenderer>(); });
        registerRenderer<SnowballEntity>([] { return std::make_unique<ProjectileEntityRenderer>(14); });
        registerRenderer<EggEntity>([] { return std::make_unique<ProjectileEntityRenderer>(12); });
        registerRenderer<FireballEntity>([] { return std::make_unique<FireballEntityRenderer>(); });
        registerRenderer<FishingBobberEntity>([] { return std::make_unique<FishingBobberEntityRenderer>(); });
        registerRenderer<PaintingEntity>([] { return std::make_unique<PaintingEntityRenderer>(); });
        registerRenderer<MinecartEntity>([] { return std::make_unique<MinecartEntityRenderer>(); });
        registerRenderer<BoatEntity>([] { return std::make_unique<BoatEntityRenderer>(); });
    }
} s_vanillaEntityRendererRegistrations;

} // namespace
