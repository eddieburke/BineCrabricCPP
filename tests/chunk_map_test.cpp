#include <gtest/gtest.h>

#include <stdexcept>

#include "net/minecraft/server/ChunkMap.hpp"
#include "net/minecraft/server/MinecraftServer.hpp"

namespace net::minecraft::test {
TEST(ChunkMap, ValidatesDistanceBounds) {
    net::minecraft::server::MinecraftServer server;
    EXPECT_THROW(net::minecraft::server::ChunkMap(&server, 0, 2), std::invalid_argument);
    EXPECT_THROW(net::minecraft::server::ChunkMap(&server, 0, 16), std::invalid_argument);
    net::minecraft::server::ChunkMap chunkMap(&server, 0, 10);
    EXPECT_EQ(chunkMap.getBlockViewDistance(), 10 * 16 - 16);
}
}  // namespace net::minecraft::test
