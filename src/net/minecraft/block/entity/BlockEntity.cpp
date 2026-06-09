#include "net/minecraft/block/entity/BlockEntity.hpp"

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/entity/ChestBlockEntity.hpp"
#include "net/minecraft/block/entity/DispenserBlockEntity.hpp"
#include "net/minecraft/block/entity/FurnaceBlockEntity.hpp"
#include "net/minecraft/block/entity/JukeboxBlockEntity.hpp"
#include "net/minecraft/block/entity/MobSpawnerBlockEntity.hpp"
#include "net/minecraft/block/entity/NoteBlockBlockEntity.hpp"
#include "net/minecraft/block/entity/PistonBlockEntity.hpp"
#include "net/minecraft/block/entity/SignBlockEntity.hpp"
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/world/event/listener/GameEventListener.hpp"

#include <functional>
#include <iostream>
#include <unordered_map>

namespace net::minecraft::block::entity {

namespace {

using Factory = std::function<std::unique_ptr<BlockEntity>()>;

std::unordered_map<std::string, Factory>& idFactories()
{
    static std::unordered_map<std::string, Factory> factories;
    static bool initialized = false;
    if (!initialized) {
        initialized = true;
        factories.emplace("Furnace", [] { return std::make_unique<FurnaceBlockEntity>(); });
        factories.emplace("Chest", [] { return std::make_unique<ChestBlockEntity>(); });
        factories.emplace("RecordPlayer", [] { return std::make_unique<JukeboxBlockEntity>(); });
        factories.emplace("Trap", [] { return std::make_unique<DispenserBlockEntity>(); });
        factories.emplace("Sign", [] { return std::make_unique<SignBlockEntity>(); });
        factories.emplace("MobSpawner", [] { return std::make_unique<MobSpawnerBlockEntity>(); });
        factories.emplace("Music", [] { return std::make_unique<NoteBlockBlockEntity>(); });
        factories.emplace("Piston", [] { return std::make_unique<PistonBlockEntity>(); });
    }
    return factories;
}

} // namespace

void BlockEntity::readNbt(const NbtCompound& nbt)
{
    x = nbt.getInt("x");
    y = nbt.getInt("y");
    z = nbt.getInt("z");
}

void BlockEntity::writeNbt(NbtCompound& nbt) const
{
    nbt.putString("id", id());
    nbt.putInt("x", x);
    nbt.putInt("y", y);
    nbt.putInt("z", z);
}

int BlockEntity::getPushedBlockData() const
{
    if (world == nullptr) {
        return 0;
    }
    return static_cast<int>(world->getBlockMeta(x, y, z));
}

Block* BlockEntity::getBlock() const
{
    if (world == nullptr) {
        return nullptr;
    }
    const int blockId = world->getBlockId(x, y, z);
    if (blockId < 0 || blockId >= Block::BLOCK_COUNT) {
        return nullptr;
    }
    return Block::BLOCKS[static_cast<std::size_t>(blockId)];
}

void BlockEntity::markDirty()
{
    if (world != nullptr) {
        world->updateBlockEntity(x, y, z, this);
    }
}

std::unique_ptr<BlockEntity> BlockEntity::createFromNbt(const NbtCompound& nbt)
{
    const std::string typeId = nbt.getString("id");
    const auto it = idFactories().find(typeId);
    if (it == idFactories().end()) {
        std::cout << "Skipping TileEntity with id " << typeId << '\n';
        return nullptr;
    }

    std::unique_ptr<BlockEntity> blockEntity = it->second();
    if (blockEntity != nullptr) {
        blockEntity->readNbt(nbt);
    }
    return blockEntity;
}

} // namespace net::minecraft::block::entity
