#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

#include "net/minecraft/client/multiplayer/ClientNetworkBridge.hpp"

#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/core/WorldSession.hpp"
#include "net/minecraft/client/network/ClientNetworkHandler.hpp"
#include "net/minecraft/network/Connection.hpp"

#include <sstream>
#include <stdexcept>
#include <utility>

namespace net::minecraft::client::multiplayer {
namespace {

SOCKET openClientSocket(const std::string& host, int port, std::string& errorOut)
{
    WSADATA wsaData {};
    const int startupResult = ::WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (startupResult != 0) {
        errorOut = "WSAStartup failed";
        return INVALID_SOCKET;
    }

    addrinfo hints {};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    std::ostringstream portStream;
    portStream << port;
    const std::string portString = portStream.str();

    addrinfo* addressResult = nullptr;
    const int resolveResult = ::getaddrinfo(host.c_str(), portString.c_str(), &hints, &addressResult);
    if (resolveResult != 0 || addressResult == nullptr) {
        errorOut = "Unknown host '" + host + "'";
        return INVALID_SOCKET;
    }

    SOCKET socket = INVALID_SOCKET;
    for (addrinfo* candidate = addressResult; candidate != nullptr; candidate = candidate->ai_next) {
        socket = ::socket(candidate->ai_family, candidate->ai_socktype, candidate->ai_protocol);
        if (socket == INVALID_SOCKET) {
            continue;
        }

        if (::connect(socket, candidate->ai_addr, static_cast<int>(candidate->ai_addrlen)) == SOCKET_ERROR) {
            ::closesocket(socket);
            socket = INVALID_SOCKET;
            continue;
        }
        break;
    }

    ::freeaddrinfo(addressResult);

    if (socket == INVALID_SOCKET) {
        errorOut = "Failed to connect";
    }
    return socket;
}

} // namespace

ClientNetworkBridge::ClientNetworkBridge(core::WorldSession* worldSession) noexcept
    : worldSession_(worldSession)
{
}

ClientNetworkBridge::~ClientNetworkBridge() = default;

bool ClientNetworkBridge::connect(client::Minecraft* minecraft, const std::string& host, int port, std::string& errorOut)
{
    disconnect();

    if (minecraft == nullptr) {
        errorOut = "Internal client error";
        return false;
    }

    const SOCKET socket = openClientSocket(host, port, errorOut);
    if (socket == INVALID_SOCKET) {
        return false;
    }

    handler_ = std::make_unique<network::ClientNetworkHandler>(minecraft);
    handler_->message = "Connecting...";

    try {
        connection_ = std::make_unique<net::minecraft::Connection>(socket, "Client", *handler_);
    } catch (const std::exception& error) {
        ::closesocket(socket);
        handler_.reset();
        errorOut = std::string("Internal client error: ") + error.what();
        return false;
    }

    handler_->bindConnection(connection_.get());
    return true;
}

void ClientNetworkBridge::disconnect(const std::string& reason)
{
    if (connection_ != nullptr) {
        connection_->disconnect();
    }

    if (handler_ != nullptr) {
        handler_->disconnect(reason);
    }

    if (worldSession_ != nullptr && handler_ != nullptr && handler_->minecraft != nullptr) {
        worldSession_->clearWorld(*handler_->minecraft);
    }

    handler_.reset();
    connection_.reset();
}

void ClientNetworkBridge::tick()
{
    if (handler_ != nullptr) {
        handler_->tick();
    }
}

network::ClientNetworkHandler* ClientNetworkBridge::handler() const noexcept
{
    return handler_.get();
}

net::minecraft::Connection* ClientNetworkBridge::connection() const noexcept
{
    return connection_.get();
}

} // namespace net::minecraft::client::multiplayer
