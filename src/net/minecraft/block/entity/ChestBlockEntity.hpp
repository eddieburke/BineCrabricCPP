#pragma once
#include <string>
#include <vector>

#include "net/minecraft/block/entity/BlockEntity.hpp"
#include "net/minecraft/block/entity/BlockEntityInventoryHelpers.hpp"
#include "net/minecraft/inventory/Inventory.hpp"
#include "net/minecraft/item/ItemStack.hpp"

namespace net::minecraft::block::entity {
class ChestBlockEntity : public BlockEntity, public Inventory {
   public:
    [[nodiscard]] std::size_t size() const override {
        return 27;
    }

    [[nodiscard]] ItemStack getStack(std::size_t slot) const override;
    [[nodiscard]] ItemStack removeStack(std::size_t slot, int amount) override;
    void setStack(std::size_t slot, ItemStack stack) override;
    [[nodiscard]] std::string getName() const override;
    void readNbt(const NbtCompound& nbt) override;
    void writeNbt(NbtCompound& nbt) const override;
    [[nodiscard]] int getMaxCountPerStack() const override;
    void markDirty() override;
    [[nodiscard]] bool canPlayerUse(PlayerEntity* player) const override;

    [[nodiscard]] std::string id() const override {
        return "Chest";
    }

   private:
    std::vector<ItemStack> inventory_ = std::vector<ItemStack>(27);
};
}  // namespace net::minecraft::block::entity
