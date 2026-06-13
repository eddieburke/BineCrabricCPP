#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/entity/BlockEntity.hpp"
#include "net/minecraft/mod/GameHooks.hpp"
#include "net/minecraft/mod/ModLifecycle.hpp"
#include "net/minecraft/nbt/NbtCompound.hpp"
#include "net/minecraft/registry/ContentRegistries.hpp"
#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/world/dimension/Dimension.hpp"

#include <cassert>

int main()
{
    using namespace net::minecraft;

    int lifecycleEvents = 0;
    mod::hooks().subscribe<mod::LifecycleEvent>(0, [&lifecycleEvents](mod::LifecycleEvent&) {
        ++lifecycleEvents;
        return mod::HookResult::continueHooks();
    });

    registry::Registry::bootstrap();
    assert(registry::Registry::isBootstrapped());
    assert(mod::ModLifecycle::frozen());
    assert(lifecycleEvents > 0);

    std::unique_ptr<Dimension> overworld = registry::DimensionRegistry::instance().create(0);
    std::unique_ptr<Dimension> nether = registry::DimensionRegistry::instance().create(-1);
    std::unique_ptr<Dimension> skylands = registry::DimensionRegistry::instance().create(1);
    assert(overworld != nullptr && overworld->id == 0);
    assert(nether != nullptr && nether->id == -1);
    assert(skylands != nullptr && skylands->id == 1);
    assert(registry::DimensionRegistry::instance().create(99) == nullptr);

    NbtCompound tile;
    tile.putString("id", "Furnace");
    tile.putInt("x", 1);
    tile.putInt("y", 2);
    tile.putInt("z", 3);
    std::unique_ptr<block::entity::BlockEntity> blockEntity =
        registry::BlockEntityRegistry::instance().create("Furnace");
    std::unique_ptr<block::entity::BlockEntity> blockEntityFromNbt =
        block::entity::BlockEntity::createFromNbt(tile);
    assert(blockEntity != nullptr);
    assert(blockEntityFromNbt != nullptr);
    assert(blockEntityFromNbt->x == 1);
    assert(blockEntityFromNbt->y == 2);
    assert(blockEntityFromNbt->z == 3);

    registry::FuelRegistry::instance().registerFuel(12345, 77);
    assert(registry::FuelRegistry::instance().burnTicks(12345).value() == 77);

    mod::hooks().subscribe<mod::FovEvent>(0, [](mod::FovEvent& event) {
        event.fov += 5.0f;
        return mod::HookResult::continueHooks();
    });
    mod::FovEvent fovEvent {nullptr, 0.0f, 70.0f};
    mod::hooks().publish(fovEvent);
    assert(fovEvent.fov == 75.0f);

    return 0;
}
