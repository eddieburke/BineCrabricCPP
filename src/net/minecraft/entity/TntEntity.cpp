#include "net/minecraft/entity/TntEntity.hpp"
#include <cmath>
#include "net/minecraft/registry/Registry.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
#include "net/minecraft/world/World.hpp"
namespace net::minecraft::entity {
TntEntity::TntEntity(World* world) : Entity(world) {
  blocksSameBlockSpawning = true;
  setBoundingBoxSpacing(0.98f, 0.98f);
  standingEyeHeight = height / 2.0f;
}
TntEntity::TntEntity(World* world, double xIn, double yIn, double zIn) : TntEntity(world) {
  setPosition(xIn, yIn, zIn);
  const float angle = static_cast<float>(random.nextDouble() * 3.1415927410125732 * 2.0);
  constexpr float kPiOver180 = 3.1415927410125732f / 180.0f;
  velocityX = -static_cast<double>(MathHelper::sin(angle * kPiOver180)) * 0.02;
  velocityY = 0.2;
  velocityZ = -static_cast<double>(MathHelper::cos(angle * kPiOver180)) * 0.02;
  fuse = 80;
  prevX = x;
  prevY = y;
  prevZ = z;
}
void TntEntity::tick() {
  prevX = x;
  prevY = y;
  prevZ = z;
  velocityY -= 0.04;
  move(velocityX, velocityY, velocityZ);
  velocityX *= 0.98;
  velocityY *= 0.98;
  velocityZ *= 0.98;
  if(onGround) {
    velocityX *= 0.7;
    velocityZ *= 0.7;
    velocityY *= -0.5;
  }
  if(--fuse <= 0) {
    if(world != nullptr && !world->isRemote()) {
      markDead();
      world->createExplosion(nullptr, x, y, z, 4.0f);
    } else {
      markDead();
    }
  } else if(world != nullptr) {
    world->addParticle("smoke", x, y + 0.5, z, 0.0, 0.0, 0.0);
  }
}
void TntEntity::writeNbt(NbtCompound& nbt) const {
  Entity::writeNbt(nbt);
  nbt.putByte("Fuse", static_cast<std::int8_t>(fuse));
}
void TntEntity::readNbt(const NbtCompound& nbt) {
  Entity::readNbt(nbt);
  fuse = nbt.getByte("Fuse");
}
MC_REGISTER_ENTITY(TntEntity)
} // namespace net::minecraft::entity
