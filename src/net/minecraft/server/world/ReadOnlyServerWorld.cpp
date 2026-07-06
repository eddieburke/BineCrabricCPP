#include "net/minecraft/server/world/ReadOnlyServerWorld.hpp"
#include "net/minecraft/server/MinecraftServer.hpp"
#include "net/minecraft/world/storage/WorldStorage.hpp"
namespace net::minecraft::server::world {
ReadOnlyServerWorld::ReadOnlyServerWorld(MinecraftServer* server, WorldStorage* storage, const std::string& saveName,
                                         int dimension, std::uint64_t seed, ServerWorld* delegate)
    : ServerWorld(server, storage, saveName, dimension, seed) {
  if(delegate != nullptr) {
    persistentStateManager.shareWith(delegate->persistentStateManager);
  }
}
} // namespace net::minecraft::server::world
