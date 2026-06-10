#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/item/BucketItem.hpp"

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/material/Material.hpp"
#include "net/minecraft/entity/passive/CowEntity.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/item/ItemPlacement.hpp"
#include "net/minecraft/item/ItemRegistrar.hpp"
#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/util/hit/HitResult.hpp"
#include "net/minecraft/util/hit/HitResultType.hpp"
#include "net/minecraft/world/World.hpp"

#include <cstdlib>
#include <optional>

namespace net::minecraft::item {

BucketItem::BucketItem(int rawId, int fluidBlockId)
    : Item(rawId),
      fluidBlockId_(fluidBlockId)
{
    setMaxCount(1);
}

ItemStack* BucketItem::use(ItemStack* stack, World* world, PlayerEntity* user)
{
    if (world == nullptr || user == nullptr) {
        return stack;
    }

    constexpr float partialTicks = 1.0f;
    Vec3d start;
    Vec3d end;
    detail::playerLookRay(user, partialTicks, start, end);
    const double soundX = user->prevX + (user->x - user->prevX) * static_cast<double>(partialTicks);
    const double soundY = user->prevY + (user->y - user->prevY) * static_cast<double>(partialTicks) + 1.62
        - static_cast<double>(user->standingEyeHeight);
    const double soundZ = user->prevZ + (user->z - user->prevZ) * static_cast<double>(partialTicks);

    std::optional<HitResult> hit = world->raycast(start, end, fluidBlockId_ == 0);
    if (!hit.has_value()) {
        return stack;
    }

    if (hit->type == HitResultType::BLOCK) {
        int x = hit->blockX;
        int y = hit->blockY;
        int z = hit->blockZ;
        if (!world->canInteract(user, x, y, z)) {
            return stack;
        }
        if (fluidBlockId_ == 0) {
            const block::material::Material& material = world->getMaterial(x, y, z);
            if (&material == &block::material::Material::WATER && world->getBlockMeta(x, y, z) == 0) {
                world->setBlock(x, y, z, 0);
                return new ItemStack(Item::WATER_BUCKET);
            }
            if (&material == &block::material::Material::LAVA && world->getBlockMeta(x, y, z) == 0) {
                world->setBlock(x, y, z, 0);
                return new ItemStack(Item::LAVA_BUCKET);
            }
        } else {
            if (fluidBlockId_ < 0) {
                return new ItemStack(Item::BUCKET);
            }
            detail::offsetPlacementPos(world, x, y, z, hit->side);
            if (world->isAir(x, y, z) || !world->getMaterial(x, y, z).isSolid()) {
                if (world->dimension != nullptr && world->dimension->evaporatesWater
                    && Block::FLOWING_WATER != nullptr && fluidBlockId_ == Block::FLOWING_WATER->id) {
                    world->playSound(
                        soundX + 0.5,
                        soundY + 0.5,
                        soundZ + 0.5,
                        "random.fizz",
                        0.5f,
                        2.6f + (world->random().nextFloat() - world->random().nextFloat()) * 0.8f);
                    for (int i = 0; i < 8; ++i) {
                        world->addParticle(
                            "largesmoke",
                            static_cast<double>(x) + static_cast<double>(std::rand()) / static_cast<double>(RAND_MAX),
                            static_cast<double>(y) + static_cast<double>(std::rand()) / static_cast<double>(RAND_MAX),
                            static_cast<double>(z) + static_cast<double>(std::rand()) / static_cast<double>(RAND_MAX),
                            0.0,
                            0.0,
                            0.0);
                    }
                } else {
                    world->setBlock(x, y, z, fluidBlockId_, 0);
                }
                return new ItemStack(Item::BUCKET);
            }
        }
    } else if (fluidBlockId_ == 0 && hit->entity != nullptr
        && dynamic_cast<entity::passive::CowEntity*>(hit->entity) != nullptr) {
        return new ItemStack(Item::MILK_BUCKET);
    }
    return stack;
}

namespace {

void BucketItem::registerClass()
{
    static BucketItem BUCKET(69, 0);
    BUCKET.setTexturePosition(10, 4)->setTranslationKey("bucket");
    Item::BUCKET = &BUCKET;

    static BucketItem WATER_BUCKET(70, Block::FLOWING_WATER != nullptr ? Block::FLOWING_WATER->id : 8);
    WATER_BUCKET.setTexturePosition(11, 4)->setTranslationKey("bucketWater")->setCraftingReturnItem(Item::BUCKET);
    Item::WATER_BUCKET = &WATER_BUCKET;

    static BucketItem LAVA_BUCKET(71, Block::FLOWING_LAVA != nullptr ? Block::FLOWING_LAVA->id : 10);
    LAVA_BUCKET.setTexturePosition(12, 4)->setTranslationKey("bucketLava")->setCraftingReturnItem(Item::BUCKET);
    Item::LAVA_BUCKET = &LAVA_BUCKET;

    static BucketItem MILK_BUCKET(79, -1);
    MILK_BUCKET.setTexturePosition(13, 4)->setTranslationKey("milk")->setCraftingReturnItem(Item::BUCKET);
    Item::MILK_BUCKET = &MILK_BUCKET;
}




static ::net::minecraft::registry::RegisterItem<BucketItem> autoReg(69);
} // namespace

} // namespace net::minecraft::item
