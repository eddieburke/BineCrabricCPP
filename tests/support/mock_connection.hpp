#pragma once
#include "net/minecraft/network/NetworkHandler.hpp"
#include "net/minecraft/network/Packet.hpp"
#include <memory>
#include <vector>
namespace net::minecraft::test {
class MockNetworkHandler : public NetworkHandler {
public:
  explicit MockNetworkHandler(bool serverSide = true) : serverSide_(serverSide) {}
  [[nodiscard]] bool isServerSide() const override {
    return serverSide_;
  }
  [[nodiscard]] const std::vector<std::unique_ptr<Packet>>& receivedPackets() const {
    return receivedPackets_;
  }
  void capturePacket(std::unique_ptr<Packet> packet) {
    receivedPackets_.push_back(std::move(packet));
  }

private:
  bool serverSide_;
  std::vector<std::unique_ptr<Packet>> receivedPackets_;
};
} // namespace net::minecraft::test
