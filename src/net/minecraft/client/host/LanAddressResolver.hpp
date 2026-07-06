#pragma once
#include <cstdint>
#include <string>
#include <vector>
namespace net::minecraft::client::host {
struct LanConnectionInfo {
  std::string primaryEndpoint;
  std::vector<std::string> availableEndpoints;
  std::string loopbackEndpoint;
};
class LanAddressResolver {
public:
  [[nodiscard]] static LanConnectionInfo resolve(std::uint16_t port);
};
} // namespace net::minecraft::client::host
