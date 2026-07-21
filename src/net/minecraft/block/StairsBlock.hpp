#pragma once
#include "net/minecraft/block/Block.hpp"
namespace net::minecraft {
class BlockView;
class World;
} // namespace net::minecraft
namespace net::minecraft::recipe {
class CraftingRecipeManager;
}
namespace net::minecraft::block {
class StairsBlock : public Block {
 public:
 static constexpr int kBlockId = 67;
 static void registerRecipes(recipe::CraftingRecipeManager& recipeManager);

 public:
 static void registerClass();
 using Block::canPlaceAt;
 Block* baseBlock = nullptr;
 StairsBlock(int id, Block& base);
 [[nodiscard]] bool isOpaque() const override {
  return false;
 }
 [[nodiscard]] bool isFullCube() const override {
  return false;
 }
 [[nodiscard]] int getRenderType() const override {
  return 10;
 }
 void updateBoundingBox(const BlockView* blockView, int x, int y, int z) override;
 [[nodiscard]] net::minecraft::Box getRenderBounds(const BlockView* blockView, int x, int y, int z) const override;
 [[nodiscard]] std::optional<net::minecraft::Box> getCollisionShape(World* world,
                                                                    int x,
                                                                    int y,
                                                                    int z) const override;
 [[nodiscard]] bool isSideVisible(const BlockView* blockView, int x, int y, int z, int side) const override;
 void addIntersectingBoundingBox(
     World* world, int x, int y, int z, const net::minecraft::Box& box, std::vector<Box>& boxes) const override;
 void randomDisplayTick(World* world, int x, int y, int z, JavaRandom& random) override;
 void onBlockBreakStart(World* world, int x, int y, int z, net::minecraft::PlayerEntity* player) override;
 void onMetadataChange(World* world, int x, int y, int z, int meta) override;
 [[nodiscard]] float getLuminance(const BlockView* blockView, int x, int y, int z) const override;
 [[nodiscard]] float getBlastResistance(net::minecraft::Entity* entity) const override;
 [[nodiscard]] int getRenderLayer() const override;
 [[nodiscard]] int getDroppedItemId(int blockMeta, JavaRandom& random) const override;
 [[nodiscard]] int getDroppedItemCount(JavaRandom& random) const override;
 [[nodiscard]] int getTexture(int side, int meta) const override;
 [[nodiscard]] int getTexture(int side) const override;
 [[nodiscard]] int getTextureId(const BlockView* blockView, int x, int y, int z, int side) const override;
 [[nodiscard]] int getTickRate() const override;
 [[nodiscard]] net::minecraft::Box getBoundingBox(World* world, int x, int y, int z) const override;
 void applyVelocity(
     World* world, int x, int y, int z, net::minecraft::Entity* entity, net::minecraft::Vec3d& velocity) override;
 [[nodiscard]] bool hasCollision() const override;
 [[nodiscard]] bool hasCollision(int meta, bool allowLiquids) const override;
 [[nodiscard]] bool canPlaceAt(World* world, int x, int y, int z) const;
 void onPlaced(World* world, int x, int y, int z) override;
 void onPlaced(World* world, int x, int y, int z, net::minecraft::PlayerEntity* placer) override;
 void onBreak(World* world, int x, int y, int z) override;
 void dropStacks(World* world, int x, int y, int z, int meta, float luck) override;
 void onSteppedOn(World* world, int x, int y, int z, net::minecraft::Entity* entity) override;
 void onTick(World* world, int x, int y, int z, JavaRandom& random) override;
 bool onUse(World* world, int x, int y, int z, net::minecraft::PlayerEntity* player) override;
 void onDestroyedByExplosion(World* world, int x, int y, int z) override;
};
} // namespace net::minecraft::block
