#pragma once
#include <string>
#include <vector>
#include "net/minecraft/block/entity/BlockEntity.hpp"
#include "net/minecraft/block/entity/BlockEntityInventoryHelpers.hpp"
#include "net/minecraft/inventory/Inventory.hpp"
#include "net/minecraft/item/ItemStack.hpp"
namespace net::minecraft::block::entity {
class FurnaceBlockEntity : public BlockEntity, public Inventory {
public:
  [[nodiscard]] std::size_t size() const override {
    return inventory_.size();
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
  [[nodiscard]] int getCookTimeDelta(int multiplier) const noexcept;
  [[nodiscard]] int getFuelTimeDelta(int multiplier);
  [[nodiscard]] bool isBurning() const noexcept;
  void tick() override;
  [[nodiscard]] std::string id() const override {
    return "Furnace";
  }
  int burnTime = 0;
  int fuelTime = 0;
  int cookTime = 0;

private:
  [[nodiscard]] bool canAcceptRecipeOutput() const;
  void craftRecipe();
  [[nodiscard]] static int getFuelTime(const ItemStack& itemStack);
  std::vector<ItemStack> inventory_ = std::vector<ItemStack>(3);
};
} // namespace net::minecraft::block::entity
