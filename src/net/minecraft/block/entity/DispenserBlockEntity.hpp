#pragma once
#include <random>
#include <string>
#include <vector>

#include "net/minecraft/block/entity/BlockEntity.hpp"
#include "net/minecraft/block/entity/BlockEntityInventoryHelpers.hpp"
#include "net/minecraft/inventory/Inventory.hpp"
#include "net/minecraft/item/ItemStack.hpp"

namespace net::minecraft::block::entity {
class DispenserBlockEntity : public BlockEntity, public Inventory {
   public:
    [[nodiscard]] std::size_t size() const override {
        return 9;
    }

    [[nodiscard]] ItemStack getStack(std::size_t slot) const override;
    [[nodiscard]] ItemStack removeStack(std::size_t slot, int amount) override;
    void setStack(std::size_t slot, ItemStack stack) override;
    [[nodiscard]] std::string getName() const override;
    [[nodiscard]] ItemStack getItemToDispense();
    void readNbt(const NbtCompound& nbt) override;
    void writeNbt(NbtCompound& nbt) const override;
    [[nodiscard]] int getMaxCountPerStack() const override;
    void markDirty() override;
    [[nodiscard]] bool canPlayerUse(PlayerEntity* player) const override;

    [[nodiscard]] std::string id() const override {
        return "Trap";
    }

   private:
    std::vector<ItemStack> inventory_ = std::vector<ItemStack>(9);
    std::mt19937 random_{std::random_device{}()};
};
}  // namespace net::minecraft::block::entity
