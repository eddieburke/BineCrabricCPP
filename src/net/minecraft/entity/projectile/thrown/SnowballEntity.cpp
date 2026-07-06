#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/entity/projectile/thrown/SnowballEntity.hpp"
#include "net/minecraft/world/World.hpp"
namespace net::minecraft::entity::projectile::thrown {
void SnowballEntity::tick() {
  Entity::tick();
  tickThrownProjectile(*this, owner, inAirTime, blockX, blockY, blockZ, blockId, inGround, removalTimer, shake,
                       false);
}
void SnowballEntity::writeNbt(NbtCompound& nbt) const {
  Entity::writeNbt(nbt);
  nbt.putShort("xTile", static_cast<std::int16_t>(blockX));
  nbt.putShort("yTile", static_cast<std::int16_t>(blockY));
  nbt.putShort("zTile", static_cast<std::int16_t>(blockZ));
  nbt.putByte("inTile", static_cast<std::int8_t>(blockId));
  nbt.putByte("shake", static_cast<std::int8_t>(shake));
  nbt.putByte("inGround", static_cast<std::int8_t>(inGround ? 1 : 0));
}
void SnowballEntity::readNbt(const NbtCompound& nbt) {
  Entity::readNbt(nbt);
  blockX = nbt.getShort("xTile");
  blockY = nbt.getShort("yTile");
  blockZ = nbt.getShort("zTile");
  blockId = static_cast<int>(nbt.getByte("inTile")) & 0xFF;
  shake = static_cast<int>(nbt.getByte("shake")) & 0xFF;
  inGround = nbt.getByte("inGround") == 1;
}
MC_REGISTER_ENTITY(SnowballEntity)
} // namespace net::minecraft::entity::projectile::thrown
