#pragma once

// M7 block port — minimal read/mutation surface for block headers.
// Block headers include this instead of World.hpp to break the block↔world
// include cycle. World (facade) implements this interface; Block::onTick(World*)
// signatures stay Java-faithful.

#include "net/minecraft/util/math/Types.hpp"
#include "net/minecraft/world/BlockView.hpp"

#include <cstdint>

namespace net::minecraft {

class World;

// Read + block-mutation port used by block/*.hpp inline bodies.
// Extends BlockView (getBlockId, getBlockMeta, getMaterial, …).
class IBlockWorld : public BlockView {
public:
    ~IBlockWorld() override = default;

    [[nodiscard]] virtual bool isRemote() const = 0;
    [[nodiscard]] virtual JavaRandom& random() = 0;

    virtual bool setBlock(int x, int y, int z, int blockId, std::uint8_t meta = 0) = 0;
    virtual void setBlockMeta(int x, int y, int z, int meta) = 0;
    virtual bool setBlockMetaWithoutNotifyingNeighbors(int x, int y, int z, int meta) = 0;
    virtual bool setBlockWithoutNotifyingNeighbors(int x, int y, int z, int blockId, int meta = 0) = 0;

    virtual void scheduleBlockUpdate(int x, int y, int z, int blockId, int tickRate) = 0;
    virtual void notifyNeighbors(int x, int y, int z, int blockId) = 0;

    [[nodiscard]] virtual int getLightLevel(int x, int y, int z) const = 0;
    [[nodiscard]] virtual int getBrightness(int x, int y, int z) const = 0;
    [[nodiscard]] virtual bool hasSkyLight(int x, int y, int z) const = 0;
    [[nodiscard]] virtual bool isAir(int x, int y, int z) const = 0;
    [[nodiscard]] virtual bool isRaining(int x, int y, int z) const = 0;
};

} // namespace net::minecraft
