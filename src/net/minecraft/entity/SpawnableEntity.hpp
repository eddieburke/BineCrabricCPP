#pragma once

namespace net::minecraft::entity {

// Marker interface matching net.minecraft.entity.SpawnableEntity.
class SpawnableEntity {
public:
    virtual ~SpawnableEntity() = default;
};

} // namespace net::minecraft::entity
