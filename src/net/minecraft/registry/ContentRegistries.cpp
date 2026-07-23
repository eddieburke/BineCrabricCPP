#include "net/minecraft/registry/ContentRegistries.hpp"
#include <cassert>
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/registry/Registry.hpp"
namespace net::minecraft::registry {
namespace {
struct FuelRegistryBootstrap {
 static void registerClass() {
  FuelRegistry::instance().collectRegisteredItems();
 }
};
static RegisterPhase<FuelRegistryBootstrap> s_fuelRegistryBootstrap(mod::LifecyclePhase::PostInit, 0);
} // namespace
BlockEntityRegistry& BlockEntityRegistry::instance() {
 static BlockEntityRegistry registry;
 return registry;
}
void BlockEntityRegistry::registerFactory(std::string legacyId, Factory factory) {
 assert(!factories_.contains(legacyId) && "BlockEntityRegistry: duplicate legacy id");
 factories_.emplace(std::move(legacyId), std::move(factory));
}
bool BlockEntityRegistry::hasFactory(const std::string& legacyId) const {
 return factories_.contains(legacyId);
}
std::unique_ptr<block::entity::BlockEntity> BlockEntityRegistry::create(const std::string& legacyId) const {
 assert(Registry::isBootstrapped() && "BlockEntityRegistry: bootstrap not complete");
 const auto it = factories_.find(legacyId);
 if(it == factories_.end()) {
  return nullptr;
 }
 return it->second();
}
FuelRegistry& FuelRegistry::instance() {
 static FuelRegistry registry;
 return registry;
}
void FuelRegistry::registerFuel(int itemId, int burnTicks) {
 fuels_[itemId] = burnTicks;
}
void FuelRegistry::registerFuel(Item* item, int burnTicks) {
 if(item != nullptr) {
  registerFuel(item->id, burnTicks);
 }
}
void FuelRegistry::collectRegisteredItems() {
 for(Item* item : Item::ITEMS) {
  if(item != nullptr && item->getFuelTime() > 0) {
   registerFuel(item->id, item->getFuelTime());
  }
 }
}
std::optional<int> FuelRegistry::burnTicks(int itemId) const {
 const auto it = fuels_.find(itemId);
 if(it == fuels_.end()) {
  return std::nullopt;
 }
 return it->second;
}
} // namespace net::minecraft::registry
