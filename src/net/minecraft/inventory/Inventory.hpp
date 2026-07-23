#pragma once
#include <cstddef>
#include <optional>
#include <string>
#include <vector>
#include "net/minecraft/entity/EntityTypes.hpp"
#include "net/minecraft/item/ItemStack.hpp"
namespace net::minecraft {
class Inventory {
 public:
 virtual ~Inventory() = default;
 [[nodiscard]] virtual std::size_t size() const = 0;
 [[nodiscard]] virtual ItemStack getStack(std::size_t slot) const = 0;
 [[nodiscard]] virtual ItemStack removeStack(std::size_t slot, int amount) = 0;
 virtual void setStack(std::size_t slot, ItemStack stack) = 0;
 [[nodiscard]] virtual std::string getName() const = 0;
 [[nodiscard]] virtual int getMaxCountPerStack() const = 0;
 virtual void markDirty() = 0;
 [[nodiscard]] virtual bool canPlayerUse(PlayerEntity* player) const = 0;
};
} // namespace net::minecraft
