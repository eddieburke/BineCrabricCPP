#pragma once

#include "net/minecraft/block/BlockWithEntity.hpp"
#include "net/minecraft/block/material/Material.hpp"

namespace net::minecraft::recipe { class CraftingRecipeManager; }
namespace net::minecraft::block {

class NoteBlock : public BlockWithEntity {
public:
    static constexpr int kBlockId = 25;
static void registerRecipes(recipe::CraftingRecipeManager& recipeManager);
public:
    static void registerClass();
    NoteBlock(int id);

    [[nodiscard]] int getTexture(int side) const override;
    void neighborUpdate(World* world, int x, int y, int z, int id) override;
    bool onUse(World* world, int x, int y, int z, net::minecraft::PlayerEntity* player) override;
    void onBlockBreakStart(World* world, int x, int y, int z, net::minecraft::PlayerEntity* player) override;
    void onBlockAction(World* world, int x, int y, int z, int data1, int data2) override;

protected:
    std::unique_ptr<entity::BlockEntity> createBlockEntity() override;
};

} // namespace net::minecraft::block
