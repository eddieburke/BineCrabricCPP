#include "net/minecraft/client/multiplayer/MultiplayerSession.hpp"

#include "net/minecraft/client/multiplayer/ClientNetworkBridge.hpp"

namespace net::minecraft::client::multiplayer {

// Out-of-line: ClientNetworkBridge is incomplete in the header, so the destructor that
// tears down bridge_/retiredBridges_ must be emitted here.
MultiplayerSession::~MultiplayerSession() = default;

void MultiplayerSession::adoptBridge(std::unique_ptr<ClientNetworkBridge> bridge)
{
    retireBridge();
    bridge_ = std::move(bridge);
}

void MultiplayerSession::retireBridge()
{
    if (bridge_ != nullptr) {
        retiredBridges_.push_back(std::move(bridge_));
    }
}

void MultiplayerSession::flushRetired()
{
    retiredBridges_.clear();
}

} // namespace net::minecraft::client::multiplayer
