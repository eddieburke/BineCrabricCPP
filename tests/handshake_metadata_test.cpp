#include <gtest/gtest.h>
#include <string>
#include <unordered_map>
#include "net/minecraft/network/HandshakeMetadata.hpp"
namespace net::minecraft::test {
TEST(HandshakeMetadata, OmegaRoundTrip) {
 const std::unordered_map<std::string, std::string> downloads{{"stone_bricks", "https://example.test/stone.zip"}};
 const auto parsed = network::parseHandshakeMetadata(
     network::appendHandshakeMetadata("-", true, true, {"stone_bricks", "iron_bars"}, downloads));
 EXPECT_TRUE(parsed.hasOmegaMetadata);
 EXPECT_EQ(parsed.serverId, "-");
 EXPECT_TRUE(parsed.nativeCppMods);
 EXPECT_TRUE(parsed.luaModsEnabled);
 ASSERT_EQ(parsed.requiredMods.size(), 2U);
 EXPECT_EQ(parsed.requiredMods[0], "stone_bricks");
 EXPECT_EQ(parsed.requiredMods[1], "iron_bars");
 EXPECT_EQ(parsed.downloadUrls.at("stone_bricks"), "https://example.test/stone.zip");
}
TEST(HandshakeMetadata, PlainDash) {
 const auto parsed = network::parseHandshakeMetadata("-");
 EXPECT_FALSE(parsed.hasOmegaMetadata);
 EXPECT_EQ(parsed.serverId, "-");
 EXPECT_TRUE(parsed.requiredMods.empty());
}
TEST(HandshakeMetadata, LegacyMods) {
 const auto parsed = network::parseHandshakeMetadata("-;mods=camera,repair_table");
 EXPECT_EQ(parsed.serverId, "-");
 ASSERT_EQ(parsed.requiredMods.size(), 2U);
 EXPECT_EQ(parsed.requiredMods[0], "camera");
 EXPECT_EQ(parsed.requiredMods[1], "repair_table");
}
} // namespace net::minecraft::test
