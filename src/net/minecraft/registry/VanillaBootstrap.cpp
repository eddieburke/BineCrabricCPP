// Vanilla content that registers as whole groups during a single lifecycle
// phase (block entities, dimensions). Both the factory bodies and the phase
// wiring live here — no shared header, since nothing else calls them.
#include "net/minecraft/block/entity/ChestBlockEntity.hpp"
#include "net/minecraft/block/entity/DispenserBlockEntity.hpp"
#include "net/minecraft/block/entity/FurnaceBlockEntity.hpp"
#include "net/minecraft/block/entity/JukeboxBlockEntity.hpp"
#include "net/minecraft/block/entity/MobSpawnerBlockEntity.hpp"
#include "net/minecraft/block/entity/NoteBlockBlockEntity.hpp"
#include "net/minecraft/block/entity/PistonBlockEntity.hpp"
#include "net/minecraft/block/entity/SignBlockEntity.hpp"
#include "net/minecraft/registry/ContentRegistries.hpp"
#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/world/dimension/NetherDimension.hpp"
#include "net/minecraft/world/dimension/OverworldDimension.hpp"
#include "net/minecraft/world/dimension/SkylandsDimension.hpp"

namespace net::minecraft::registry {
namespace {

void registerVanillaBlockEntities()
{
    BlockEntityRegistry& registry = BlockEntityRegistry::instance();
    registry.registerFactory("Furnace", [] { return std::make_unique<block::entity::FurnaceBlockEntity>(); });
    registry.registerFactory("Chest", [] { return std::make_unique<block::entity::ChestBlockEntity>(); });
    registry.registerFactory("RecordPlayer", [] { return std::make_unique<block::entity::JukeboxBlockEntity>(); });
    registry.registerFactory("Trap", [] { return std::make_unique<block::entity::DispenserBlockEntity>(); });
    registry.registerFactory("Sign", [] { return std::make_unique<block::entity::SignBlockEntity>(); });
    registry.registerFactory("MobSpawner", [] { return std::make_unique<block::entity::MobSpawnerBlockEntity>(); });
    registry.registerFactory("Music", [] { return std::make_unique<block::entity::NoteBlockBlockEntity>(); });
    registry.registerFactory("Piston", [] { return std::make_unique<block::entity::PistonBlockEntity>(); });
}

void registerVanillaDimensions()
{
    DimensionRegistry& registry = DimensionRegistry::instance();
    registry.registerFactory(-1, [] { return std::make_unique<NetherDimension>(); });
    registry.registerFactory(0, [] { return std::make_unique<OverworldDimension>(); });
    registry.registerFactory(1, [] { return std::make_unique<SkylandsDimension>(); });
}

struct VanillaBlockEntityBootstrap {
    static void registerClass() { registerVanillaBlockEntities(); }
};

struct VanillaDimensionBootstrap {
    static void registerClass() { registerVanillaDimensions(); }
};

static RegisterPhase<VanillaBlockEntityBootstrap> s_blockEntities(mod::LifecyclePhase::BlockEntityRegistration, 0);
static RegisterPhase<VanillaDimensionBootstrap> s_dimensions(mod::LifecyclePhase::DimensionRegistration, 0);

} // namespace
} // namespace net::minecraft::registry
