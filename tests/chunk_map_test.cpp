#include "net/minecraft/server/ChunkMap.hpp"
#include "net/minecraft/server/MinecraftServer.hpp"
#include <cassert>
#include <stdexcept>
int main() {
  net::minecraft::server::MinecraftServer server;
  try {
    net::minecraft::server::ChunkMap tooSmall(&server, 0, 2);
    assert(false);
  } catch(const std::invalid_argument&) {
  }
  try {
    net::minecraft::server::ChunkMap tooLarge(&server, 0, 16);
    assert(false);
  } catch(const std::invalid_argument&) {
  }
  net::minecraft::server::ChunkMap chunkMap(&server, 0, 10);
  assert(chunkMap.getBlockViewDistance() == 10 * 16 - 16);
  return 0;
}
