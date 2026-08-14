#pragma once
#include <cstdint>
#include <optional>
#include <string>
namespace net::minecraft::client {
struct StartupOptions {
 std::string world;
 std::int64_t seed = 0;
 bool debugHud = false;
 int width = 854;
 int height = 480;
 bool headless = false;
};
struct ClientLaunchOptions {
 std::string username;
 std::string sessionId = "-";
 std::optional<std::string> server;
 StartupOptions startup;
};
[[nodiscard]] ClientLaunchOptions parseClientLaunchOptions(int argc,
                                                           const char* const* argv,
                                                           std::string defaultUsername);
} // namespace net::minecraft::client
