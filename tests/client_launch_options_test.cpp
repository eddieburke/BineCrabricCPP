#include <gtest/gtest.h>
#include <stdexcept>
#include "net/minecraft/client/ClientLaunchOptions.hpp"
namespace net::minecraft::test {
TEST(ClientLaunchOptionsTest, ParsesHeadlessWorldAndDimensions) {
 const char* argv[] = {"minecraft", "--headless", "--world", "World1",
                       "--width", "1280", "--height", "720"};
 const auto launch = client::parseClientLaunchOptions(8, argv, "Fallback");
 EXPECT_TRUE(launch.startup.headless);
 EXPECT_EQ(launch.startup.world, "World1");
 EXPECT_EQ(launch.startup.width, 1280);
 EXPECT_EQ(launch.startup.height, 720);
}
TEST(ClientLaunchOptionsTest, RejectsUnknownBenchmarkOption) {
 const char* argv[] = {"minecraft", "--benchmark-frames", "60"};
 EXPECT_THROW((void)client::parseClientLaunchOptions(3, argv, "Fallback"), std::runtime_error);
}
TEST(ClientLaunchOptionsTest, ParsesNamedIdentityAndServer) {
 const char* argv[] = {"minecraft", "--username", "Eddie", "--session", "token",
                       "--server", "localhost:25565"};
 const auto launch = client::parseClientLaunchOptions(7, argv, "Fallback");
 EXPECT_EQ(launch.username, "Eddie");
 EXPECT_EQ(launch.sessionId, "token");
 ASSERT_TRUE(launch.server.has_value());
 EXPECT_EQ(*launch.server, "localhost:25565");
}
} // namespace net::minecraft::test
