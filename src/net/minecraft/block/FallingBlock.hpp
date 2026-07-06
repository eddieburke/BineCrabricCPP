#pragma once
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/material/Material.hpp"
namespace net::minecraft::block {
class FallingBlock : public Block {
public:
  static thread_local bool fallInstantly;
  [[nodiscard]] int getTickRate() const override {
    return 3;
  }
  void onPlaced(World* world, int x, int y, int z) override;
  void neighborUpdate(World* world, int x, int y, int z, int id) override;
  void onTick(World* world, int x, int y, int z, JavaRandom& random) override;
  static bool canFallThrough(World* world, int x, int y, int z);

protected:
  FallingBlock(int id, int textureId, Material& material) : Block(id, textureId, material) {}

private:
  void processFall(World* world, int x, int y, int z);
};
} // namespace net::minecraft::block
