#pragma once
#include <cstdint>
namespace net::minecraft {
class World;
// M4 vertical slice: setBlock*, neighbor propagation, placement checks, post-mutation hooks.
// Light enqueue remains in Chunk::setBlock (queueLightUpdate); renderer dirty via World::setBlockDirty.
class BlockMutationModule {
 public:
 explicit BlockMutationModule(World& world) noexcept : world_(world) {
 }
 bool setBlock(int x, int y, int z, int blockId, std::uint8_t meta = 0);
 void setBlockMeta(int x, int y, int z, int meta);
 bool setBlockMetaWithoutNotifyingNeighbors(int x, int y, int z, int meta);
 bool setBlockWithoutNotifyingNeighbors(int x, int y, int z, int blockId, int meta);
 bool setBlockWithoutNotifyingNeighbors(int x, int y, int z, int blockId);
 void blockUpdate(int x, int y, int z, int blockId);
 void neighborUpdate(int x, int y, int z, int sourceBlockId);
 void notifyNeighbors(int x, int y, int z, int blockId);
 [[nodiscard]] bool canPlace(int blockId, int x, int y, int z, bool fallingBlock, int side);

 private:
 World& world_;
 [[nodiscard]] World& world() noexcept;
 [[nodiscard]] const World& world() const noexcept;
};
} // namespace net::minecraft
