#pragma once
#include "net/minecraft/server/MinecraftServer.hpp"
#include "net/minecraft/server/world/ServerWorldEventListener.hpp"
#include "net/minecraft/world/ServerWorld.hpp"
#include "net/minecraft/world/storage/EmptyWorldStorage.hpp"

struct ServerEventFixture {
    net::minecraft::server::MinecraftServer server;
    net::minecraft::EmptyWorldStorage storage;
    net::minecraft::ServerWorld world{&server, &storage, "test", 0, 42};
    net::minecraft::server::world::ServerWorldEventListener listener{&server, &world};

    ServerEventFixture() {
        server.worlds[0] = &world;
        server.playerManager.configureFromProperties();
        world.addEventListener(&listener);
    }
};
