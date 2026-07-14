#pragma once
#include <cstdint>
#include <string>
#include <vector>
namespace net::minecraft::client::host {
struct ServerConnectionInfo {
  std::string primaryEndpoint;
  std::vector<std::string> availableEndpoints;
  std::string loopbackEndpoint;
};
class ServerAddressResolver {
public:
  [[nodiscard]] static ServerConnectionInfo resolve(std::uint16_t port);
};
} // namespace net::minecraft::client::host
