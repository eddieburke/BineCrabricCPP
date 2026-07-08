#pragma once
#include "net/minecraft/world/ServerWorld.hpp"

namespace net::minecraft {
class WorldStorage;

namespace server {
class MinecraftServer;

namespace world {
class ReadOnlyServerWorld : public ServerWorld {
   public:
    ReadOnlyServerWorld(MinecraftServer* server,
                        WorldStorage* storage,
                        const std::string& saveName,
                        int dimension,
                        std::uint64_t seed,
                        ServerWorld* delegate);
};
}  // namespace world
}  // namespace server
}  // namespace net::minecraft
