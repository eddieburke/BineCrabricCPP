#include "net/minecraft/block/entity/FurnaceBlockEntity.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/FurnaceBlock.hpp"
#include "net/minecraft/block/material/Material.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/recipe/SmeltingRecipeManager.hpp"
#include "net/minecraft/registry/ContentRegistries.hpp"
#include "net/minecraft/world/World.hpp"
namespace net::minecraft::block::entity {
ItemStack FurnaceBlockEntity::getStack(std::size_t slot) const {
  if(slot >= inventory_.size()) {
    return {};
  }
  return inventory_[slot];
}
ItemStack FurnaceBlockEntity::removeStack(std::size_t slot, int amount) {
  if(slot >= inventory_.size()) {
    return {};
  }
  ItemStack& stack = inventory_[slot];
  if(stack.empty()) {
    return {};
  }
  if(stack.count <= amount) {
    ItemStack removed = stack;
    stack = {};
    return removed;
  }
  ItemStack removed = stack.split(amount);
  if(stack.count == 0) {
    stack = {};
  }
  return removed;
}
void FurnaceBlockEntity::setStack(std::size_t slot, ItemStack stack) {
  if(slot >= inventory_.size()) {
    return;
  }
  if(!stack.empty() && stack.count > getMaxCountPerStack()) {
    stack.count = getMaxCountPerStack();
  }
  inventory_[slot] = std::move(stack);
}
std::string FurnaceBlockEntity::getName() const {
  return "Furnace";
}
void FurnaceBlockEntity::readNbt(const NbtCompound& nbt) {
  BlockEntity::readNbt(nbt);
  inventory_.assign(inventory_.size(), {});
  inventory_util::readInventoryItems(nbt, inventory_, false);
  burnTime = nbt.getShort("BurnTime");
  cookTime = nbt.getShort("CookTime");
  fuelTime = getFuelTime(inventory_[1]);
}
void FurnaceBlockEntity::writeNbt(NbtCompound& nbt) const {
  BlockEntity::writeNbt(nbt);
  nbt.putShort("BurnTime", static_cast<std::int16_t>(burnTime));
  nbt.putShort("CookTime", static_cast<std::int16_t>(cookTime));
  inventory_util::writeInventoryItems(nbt, inventory_);
}
int FurnaceBlockEntity::getMaxCountPerStack() const {
  return 64;
}
void FurnaceBlockEntity::markDirty() {
  BlockEntity::markDirty();
}
bool FurnaceBlockEntity::canPlayerUse(PlayerEntity* player) const {
  return inventory_util::canPlayerUseBlockEntity(*this, player);
}
int FurnaceBlockEntity::getCookTimeDelta(int multiplier) const noexcept {
  return cookTime * multiplier / 200;
}
int FurnaceBlockEntity::getFuelTimeDelta(int multiplier) {
  if(fuelTime == 0) {
    fuelTime = 200;
  }
  return burnTime * multiplier / fuelTime;
}
bool FurnaceBlockEntity::isBurning() const noexcept {
  return burnTime > 0;
}
bool FurnaceBlockEntity::canAcceptRecipeOutput() const {
  if(inventory_[0].empty()) {
    return false;
  }
  const std::optional<ItemStack> recipe = recipe::SmeltingRecipeManager::instance().craft(inventory_[0].itemId);
  if(!recipe.has_value()) {
    return false;
  }
  if(inventory_[2].empty()) {
    return true;
  }
  if(!inventory_[2].isItemEqual(*recipe)) {
    return false;
  }
  if(inventory_[2].count < getMaxCountPerStack() && inventory_[2].count < inventory_[2].getMaxCount()) {
    return true;
  }
  return inventory_[2].count < recipe->getMaxCount();
}
void FurnaceBlockEntity::craftRecipe() {
  if(!canAcceptRecipeOutput()) {
    return;
  }
  const std::optional<ItemStack> recipe = recipe::SmeltingRecipeManager::instance().craft(inventory_[0].itemId);
  if(!recipe.has_value()) {
    return;
  }
  if(inventory_[2].empty()) {
    inventory_[2] = recipe->copy();
  } else if(inventory_[2].itemId == recipe->itemId) {
    ++inventory_[2].count;
  }
  --inventory_[0].count;
  if(inventory_[0].count <= 0) {
    inventory_[0] = {};
  }
}
int FurnaceBlockEntity::getFuelTime(const ItemStack& itemStack) {
  if(itemStack.empty()) {
    return 0;
  }
  const int id = itemStack.itemId;
  if(const std::optional<int> registeredFuel = registry::FuelRegistry::instance().burnTicks(id)) {
    return *registeredFuel;
  }
  if(id < 256) {
    Block* block = Block::BLOCKS[static_cast<std::size_t>(id)];
    if(block != nullptr && &block->material == &block::material::Material::WOOD) {
      return 300;
    }
  }
  return 0;
}
void FurnaceBlockEntity::tick() {
  const bool wasBurning = burnTime > 0;
  bool changed = false;
  if(burnTime > 0) {
    --burnTime;
  }
  if(world != nullptr && !world->isRemote()) {
    if(burnTime == 0 && canAcceptRecipeOutput()) {
      fuelTime = burnTime = getFuelTime(inventory_[1]);
      if(burnTime > 0) {
        changed = true;
        if(!inventory_[1].empty()) {
          --inventory_[1].count;
          if(inventory_[1].count == 0) {
            inventory_[1] = {};
          }
        }
      }
    }
    if(isBurning() && canAcceptRecipeOutput()) {
      ++cookTime;
      if(cookTime == 200) {
        cookTime = 0;
        craftRecipe();
        changed = true;
      }
    } else {
      cookTime = 0;
    }
    if(wasBurning != (burnTime > 0)) {
      changed = true;
      FurnaceBlock::updateLitState(burnTime > 0, world, x, y, z);
    }
  }
  if(changed) {
    markDirty();
  }
}
} // namespace net::minecraft::block::entity
