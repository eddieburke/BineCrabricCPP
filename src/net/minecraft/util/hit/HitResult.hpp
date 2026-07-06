#pragma once
// Faithful port of net.minecraft.util.hit.HitResult (beta 1.7.3).
// Entity is forward-declared; the entity constructor body lives in a TU that
// has the full Entity type (or is left inline-safe via the pointer member).
#include "net/minecraft/entity/EntityTypes.hpp"
#include "net/minecraft/util/hit/HitResultType.hpp"
#include "net/minecraft/util/math/Types.hpp" // Vec3d
namespace net::minecraft {
class HitResult {
public:
  HitResultType type;
  int blockX = 0;
  int blockY = 0;
  int blockZ = 0;
  int side = 0;
  Vec3d pos;
  Entity* entity = nullptr;
  HitResult(int blockX, int blockY, int blockZ, int side, const Vec3d& pos)
      : type(HitResultType::BLOCK), blockX(blockX), blockY(blockY), blockZ(blockZ), side(side),
        pos(pos.x, pos.y, pos.z) {
  }
  // Entity hit: pos is set from the entity's position by the caller/out-of-line
  // body to avoid needing the full Entity type here.
  explicit HitResult(Entity* entity, const Vec3d& entityPos)
      : type(HitResultType::ENTITY), pos(entityPos), entity(entity) {
  }
};
} // namespace net::minecraft
