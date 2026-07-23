#pragma once
namespace net::minecraft::entity {
// Port of net.minecraft.entity.SpawnableEntity (beta 1.7.3): server-side marker for
// mobs replicated via LivingEntitySpawnS2CPacket.
class SpawnableEntity {
 public:
 virtual ~SpawnableEntity() = default;
};
} // namespace net::minecraft::entity
