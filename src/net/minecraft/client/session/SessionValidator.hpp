#pragma once
#include <atomic>
#include <cstdint>
namespace net::minecraft::client {
class Minecraft;
}
namespace net::minecraft::client::session {
/// HTTP session validation at tick 6000 (`SessionCheckThread`).
/// Anchor: Minecraft.cpp L121–137, L866–868.
class SessionValidator {
public:
  inline static std::atomic<std::int64_t> failedSessionCheckTime{0};
  static void startSessionCheck(Minecraft& client);
};
} // namespace net::minecraft::client::session
