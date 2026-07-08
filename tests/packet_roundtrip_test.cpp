#include <gtest/gtest.h>

#include <sstream>

#include "net/minecraft/network/Packet.hpp"
#include "net/minecraft/network/packet/ConnectionPackets.hpp"
#include "net/minecraft/network/packet/PlayerPackets.hpp"

namespace net::minecraft::test {
TEST(PacketRegistry, KeepAliveRoundTrip) {
    Packet::ensureRegistered();
    KeepAlivePacket original;
    std::ostringstream out;
    Packet::write(original, out);
    std::istringstream in(out.str());
    const std::unique_ptr<Packet> decoded = Packet::read(in, true);
    ASSERT_NE(decoded, nullptr);
    EXPECT_EQ(decoded->rawId(), 0);
}

TEST(PacketRegistry, LoginHelloRoundTrip) {
    Packet::ensureRegistered();
    LoginHelloPacket original("Steve", 14);
    original.worldSeed = 12345ULL;
    original.dimensionId = 0;
    std::ostringstream out;
    Packet::write(original, out);
    std::istringstream in(out.str());
    const std::unique_ptr<Packet> decoded = Packet::read(in, true);
    ASSERT_NE(decoded, nullptr);
    const auto* hello = dynamic_cast<const LoginHelloPacket*>(decoded.get());
    ASSERT_NE(hello, nullptr);
    EXPECT_EQ(hello->protocolVersion, 14);
    EXPECT_EQ(hello->username, "Steve");
    EXPECT_EQ(hello->worldSeed, 12345ULL);
    EXPECT_EQ(hello->dimensionId, 0);
}

// Player-move packets carry feet (y) and stance (eyeHeight) as distinct wire fields.
// Round-trip through the wire codec to confirm they stay distinct.
TEST(PacketRegistry, ClientboundPlayerMoveKeepsFeetAndStanceDistinct) {
    Packet::ensureRegistered();
    PlayerMoveFullPacket original;
    original.setMove(12.5, 69.0, 70.62, -4.25, 90.0f, 12.0f, false);
    std::ostringstream out;
    Packet::write(original, out);
    std::istringstream in(out.str());
    const std::unique_ptr<Packet> decoded = Packet::read(in, true);
    ASSERT_NE(decoded, nullptr);
    const auto* move = dynamic_cast<const PlayerMoveFullPacket*>(decoded.get());
    ASSERT_NE(move, nullptr);
    EXPECT_DOUBLE_EQ(move->x, 12.5);
    EXPECT_DOUBLE_EQ(move->feetY, 69.0);
    EXPECT_DOUBLE_EQ(move->stance, 70.62);
    EXPECT_DOUBLE_EQ(move->z, -4.25);
    EXPECT_FLOAT_EQ(move->yaw, 90.0f);
    EXPECT_FLOAT_EQ(move->pitch, 12.0f);
    EXPECT_TRUE(move->changePosition);
    EXPECT_TRUE(move->changeLook);
}

TEST(PacketRegistry, ServerboundPlayerMoveKeepsFeetAndStanceDistinct) {
    Packet::ensureRegistered();
    PlayerMoveFullPacket original;
    original.setMove(12.5, 69.0, 70.62, -4.25, 180.0f, -5.0f, true);
    std::ostringstream out;
    Packet::write(original, out);
    std::istringstream in(out.str());
    const std::unique_ptr<Packet> decoded = Packet::read(in, true);
    ASSERT_NE(decoded, nullptr);
    const auto* move = dynamic_cast<const PlayerMoveFullPacket*>(decoded.get());
    ASSERT_NE(move, nullptr);
    EXPECT_DOUBLE_EQ(move->x, 12.5);
    EXPECT_DOUBLE_EQ(move->feetY, 69.0);
    EXPECT_DOUBLE_EQ(move->stance, 70.62);
    EXPECT_DOUBLE_EQ(move->z, -4.25);
    EXPECT_FLOAT_EQ(move->yaw, 180.0f);
    EXPECT_FLOAT_EQ(move->pitch, -5.0f);
    EXPECT_TRUE(move->changePosition);
    EXPECT_TRUE(move->changeLook);
}
}  // namespace net::minecraft::test
