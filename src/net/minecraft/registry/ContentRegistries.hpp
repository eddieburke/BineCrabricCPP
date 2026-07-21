#pragma once
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
namespace net::minecraft {
class Item;
class World;
namespace block {
class Block;
namespace entity {
class BlockEntity;
}
} // namespace block
namespace entity {
class Entity;
}
} // namespace net::minecraft
namespace net::minecraft::registry {
class BlockEntityRegistry {
 public:
 using Factory = std::function<std::unique_ptr<block::entity::BlockEntity>()>;
 static BlockEntityRegistry& instance();
 void registerFactory(std::string legacyId, Factory factory);
 [[nodiscard]] bool hasFactory(const std::string& legacyId) const;
 [[nodiscard]] std::unique_ptr<block::entity::BlockEntity> create(const std::string& legacyId) const;

 private:
 std::unordered_map<std::string, Factory> factories_;
};
// Furnace fuel lookup. Vanilla items declare burn time via Item::setFuelTime() in registerClass();
// FuelRegistration collects them with collectRegisteredItems(). Mods may also call registerFuel() directly.
class FuelRegistry {
 public:
 static FuelRegistry& instance();
 void registerFuel(int itemId, int burnTicks);
 void registerFuel(Item* item, int burnTicks);
 void collectRegisteredItems();
 [[nodiscard]] std::optional<int> burnTicks(int itemId) const;

 private:
 std::unordered_map<int, int> fuels_;
};
} // namespace net::minecraft::registry
