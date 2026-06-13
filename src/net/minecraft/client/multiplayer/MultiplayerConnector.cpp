#include "net/minecraft/client/multiplayer/MultiplayerConnector.hpp"

#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/multiplayer/MultiplayerSession.hpp"
#include "net/minecraft/client/network/ClientNetworkHandler.hpp"
#include "net/minecraft/client/resource/language/I18n.hpp"
#include "net/minecraft/network/Connection.hpp"
#include "net/minecraft/network/packet/ConnectionPackets.hpp"

#include <utility>

namespace net::minecraft::client::multiplayer {

MultiplayerConnector::MultiplayerConnector(Minecraft* minecraft, std::string host, int port)
{
    if (minecraft == nullptr) {
        return;
    }

    minecraft->setWorld(nullptr);
    thread_ = std::thread([this, minecraft, host = std::move(host), port]() {
        if (cancelled_.load(std::memory_order_acquire)) {
            return;
        }

        std::string connectError;
        auto bridge = std::make_unique<ClientNetworkBridge>(&minecraft->worldSession());
        if (!bridge->connect(minecraft, host, port, connectError)) {
            if (cancelled_.load(std::memory_order_acquire)) {
                return;
            }
            std::lock_guard lock(mutex_);
            connectError_ = connectError.empty() ? "Failed to connect" : std::move(connectError);
            return;
        }

        if (cancelled_.load(std::memory_order_acquire)) {
            bridge->disconnect();
            return;
        }

        if (net::minecraft::Connection* connection = bridge->connection()) {
            connection->sendPacket<HandshakePacket>(minecraft->session.username);
        }
        if (network::ClientNetworkHandler* handler = bridge->handler()) {
            handler->message = resource::language::I18n::getTranslation("connect.authorizing");
        }

        std::lock_guard lock(mutex_);
        pendingBridge_ = std::move(bridge);
    });
}

MultiplayerConnector::~MultiplayerConnector()
{
    cancel();
    if (thread_.joinable()) {
        thread_.join();
    }
}

void MultiplayerConnector::cancel()
{
    cancelled_.store(true, std::memory_order_release);
}

void MultiplayerConnector::disconnectActive(Minecraft& client)
{
    cancelled_.store(true, std::memory_order_release);

    ClientNetworkBridge* bridge = nullptr;
    {
        std::lock_guard lock(mutex_);
        bridge = pendingBridge_ != nullptr ? pendingBridge_.get() : client.multiplayerSession().bridge();
    }
    if (bridge != nullptr) {
        bridge->disconnect();
    }
}

std::string MultiplayerConnector::poll(Minecraft& client)
{
    std::lock_guard lock(mutex_);
    if (connectError_.has_value()) {
        std::string error = std::move(*connectError_);
        connectError_.reset();
        return error.empty() ? "Failed to connect" : std::move(error);
    }

    if (pendingBridge_ != nullptr) {
        client.multiplayerSession().adoptBridge(std::move(pendingBridge_));
    }
    return {};
}

void MultiplayerConnector::tickBridge(Minecraft& client)
{
    if (cancelled_.load(std::memory_order_acquire)) {
        return;
    }
    if (ClientNetworkBridge* bridge = client.multiplayerSession().bridge()) {
        bridge->tick();
    }
}

ClientNetworkBridge* MultiplayerConnector::activeBridge(Minecraft* client) const
{
    if (client == nullptr) {
        return nullptr;
    }

    std::lock_guard lock(mutex_);
    if (pendingBridge_ != nullptr) {
        return pendingBridge_.get();
    }
    return client->multiplayerSession().bridge();
}

} // namespace net::minecraft::client::multiplayer
