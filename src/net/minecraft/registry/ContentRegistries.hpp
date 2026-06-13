#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <vector>

namespace net::minecraft {
class Dimension;
class Item;
class World;
namespace block {
class Block;
namespace entity {
class BlockEntity;
}
}
namespace entity {
class Entity;
}
}

namespace net::minecraft::registry {

struct EntityTypeInfo {
    std::string legacyId;
    int rawId = 0;
    std::type_index type = std::type_index(typeid(void));
    int trackingRange = 64;
    int updateInterval = 3;
    bool sendsVelocityUpdates = true;
};

class EntityTypeRegistry {
public:
    static EntityTypeRegistry& instance();

    void record(std::type_index type, std::string legacyId, int rawId);
    void registerMetadata(EntityTypeInfo info);

    [[nodiscard]] const EntityTypeInfo* byRawId(int rawId) const;
    [[nodiscard]] const EntityTypeInfo* byLegacyId(const std::string& legacyId) const;
    [[nodiscard]] const EntityTypeInfo* byType(std::type_index type) const;

private:
    std::vector<EntityTypeInfo> entries_;
    std::unordered_map<int, std::size_t> rawToIndex_;
    std::unordered_map<std::string, std::size_t> legacyToIndex_;
    std::unordered_map<std::type_index, std::size_t> typeToIndex_;
};

class DimensionRegistry {
public:
    using Factory = std::function<std::unique_ptr<Dimension>()>;

    static DimensionRegistry& instance();

    void registerFactory(int numericId, Factory factory);
    [[nodiscard]] std::unique_ptr<Dimension> create(int numericId) const;
    [[nodiscard]] bool contains(int numericId) const;

private:
    std::unordered_map<int, Factory> factories_;
};

class BlockEntityRegistry {
public:
    using Factory = std::function<std::unique_ptr<block::entity::BlockEntity>()>;

    static BlockEntityRegistry& instance();

    void registerFactory(std::string legacyId, Factory factory);
    [[nodiscard]] std::unique_ptr<block::entity::BlockEntity> create(const std::string& legacyId) const;

private:
    std::unordered_map<std::string, Factory> factories_;
};

class FuelRegistry {
public:
    static FuelRegistry& instance();

    void registerFuel(int itemId, int burnTicks);
    [[nodiscard]] std::optional<int> burnTicks(int itemId) const;

private:
    std::unordered_map<int, int> fuels_;
};

} // namespace net::minecraft::registry
