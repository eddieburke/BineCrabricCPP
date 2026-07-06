#include "net/minecraft/network/Packet.hpp"
#include "net/minecraft/network/packet/ConnectionPackets.hpp"
#include <gtest/gtest.h>
#include <sstream>
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
} // namespace net::minecraft::test
