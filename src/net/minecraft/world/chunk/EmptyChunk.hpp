#pragma once
#include "net/minecraft/world/chunk/Chunk.hpp"
namespace net::minecraft {
class EmptyChunk : public Chunk {
 public:
 explicit EmptyChunk(World* world = nullptr, int x = 0, int z = 0) : Chunk(world, x, z) {
  empty = true;
 }
};
} // namespace net::minecraft
