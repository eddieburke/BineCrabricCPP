#include "net/minecraft/registry/ContentRegistries.hpp"

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/registry/Registry.hpp"

#include <cassert>

namespace net::minecraft::registry {

EntityTypeRegistry& EntityTypeRegistry::instance()
{
    static EntityTypeRegistry registry;
    return registry;
}

void EntityTypeRegistry::record(std::type_index type, std::string legacyId, int rawId)
{
    EntityTypeInfo info;
    info.legacyId = std::move(legacyId);
    info.rawId = rawId;
    info.type = type;
    registerMetadata(std::move(info));
}

void EntityTypeRegistry::registerMetadata(EntityTypeInfo info)
{
    assert(!rawToIndex_.contains(info.rawId) && "EntityTypeRegistry: duplicate raw id");
    assert(!legacyToIndex_.contains(info.legacyId) && "EntityTypeRegistry: duplicate legacy id");
    assert(!typeToIndex_.contains(info.type) && "EntityTypeRegistry: duplicate type");
    const std::size_t index = entries_.size();
    rawToIndex_.emplace(info.rawId, index);
    legacyToIndex_.emplace(info.legacyId, index);
    typeToIndex_.emplace(info.type, index);
    entries_.push_back(std::move(info));
}

const EntityTypeInfo* EntityTypeRegistry::byRawId(int rawId) const
{
    const auto it = rawToIndex_.find(rawId);
    return it == rawToIndex_.end() ? nullptr : &entries_[it->second];
}

const EntityTypeInfo* EntityTypeRegistry::byLegacyId(const std::string& legacyId) const
{
    const auto it = legacyToIndex_.find(legacyId);
    return it == legacyToIndex_.end() ? nullptr : &entries_[it->second];
}

const EntityTypeInfo* EntityTypeRegistry::byType(std::type_index type) const
{
    const auto it = typeToIndex_.find(type);
    return it == typeToIndex_.end() ? nullptr : &entries_[it->second];
}

DimensionRegistry& DimensionRegistry::instance()
{
    static DimensionRegistry registry;
    return registry;
}

void DimensionRegistry::registerFactory(int numericId, Factory factory)
{
    assert(!factories_.contains(numericId) && "DimensionRegistry: duplicate dimension id");
    factories_.emplace(numericId, std::move(factory));
}

std::unique_ptr<Dimension> DimensionRegistry::create(int numericId) const
{
    assert(Registry::isBootstrapped() && "DimensionRegistry: bootstrap not complete");
    const auto it = factories_.find(numericId);
    if (it == factories_.end()) {
        return nullptr;
    }
    std::unique_ptr<Dimension> dimension = it->second();
    if (dimension != nullptr) {
        dimension->id = numericId;
    }
    return dimension;
}

bool DimensionRegistry::contains(int numericId) const
{
    assert(Registry::isBootstrapped() && "DimensionRegistry: bootstrap not complete");
    return factories_.contains(numericId);
}

BlockEntityRegistry& BlockEntityRegistry::instance()
{
    static BlockEntityRegistry registry;
    return registry;
}

void BlockEntityRegistry::registerFactory(std::string legacyId, Factory factory)
{
    assert(!factories_.contains(legacyId) && "BlockEntityRegistry: duplicate legacy id");
    factories_.emplace(std::move(legacyId), std::move(factory));
}

std::unique_ptr<block::entity::BlockEntity> BlockEntityRegistry::create(const std::string& legacyId) const
{
    assert(Registry::isBootstrapped() && "BlockEntityRegistry: bootstrap not complete");
    const auto it = factories_.find(legacyId);
    if (it == factories_.end()) {
        return nullptr;
    }
    return it->second();
}

FuelRegistry& FuelRegistry::instance()
{
    static FuelRegistry registry;
    return registry;
}

void FuelRegistry::registerFuel(int itemId, int burnTicks)
{
    fuels_[itemId] = burnTicks;
}

std::optional<int> FuelRegistry::burnTicks(int itemId) const
{
    const auto it = fuels_.find(itemId);
    if (it == fuels_.end()) {
        return std::nullopt;
    }
    return it->second;
}

} // namespace net::minecraft::registry
