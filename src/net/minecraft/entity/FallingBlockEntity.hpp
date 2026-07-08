#pragma once
#include "net/minecraft/entity/Entity.hpp"
#include "net/minecraft/entity/EntityClientRendererDecl.hpp"

namespace net::minecraft::entity {
class FallingBlockEntity : public Entity {
   public:
    static constexpr int kEntityId = 21;
    static constexpr const char* kEntityName = "FallingSand";

    struct ClientRenderer {
        static std::unique_ptr<::net::minecraft::client::render::entity::EntityRenderer> create();
    };

    explicit FallingBlockEntity(World* world = nullptr);
    FallingBlockEntity(World* world, double x, double y, double z, int blockIdIn);
    void tick() override;

    [[nodiscard]] bool isCollidable() const override {
        return !dead;
    }

    [[nodiscard]] bool bypassesSteppingEffects() const override {
        return false;
    }

    void writeNbt(NbtCompound& nbt) const override {
        Entity::writeNbt(nbt);
        nbt.putByte("Tile", static_cast<std::int8_t>(blockId));
    }

    void readNbt(const NbtCompound& nbt) override {
        Entity::readNbt(nbt);
        blockId = static_cast<int>(nbt.getByte("Tile")) & 0xFF;
    }

    int blockId = 0;
    int timeFalling = 0;
};
}  // namespace net::minecraft::entity
