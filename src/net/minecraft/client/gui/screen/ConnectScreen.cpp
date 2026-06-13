#include "net/minecraft/client/gui/screen/ConnectScreen.hpp"

#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/gui/screen/DisconnectedScreen.hpp"
#include "net/minecraft/client/network/ClientNetworkHandler.hpp"
#include "net/minecraft/client/resource/language/I18n.hpp"
#include "net/minecraft/network/Connection.hpp"
#include "net/minecraft/network/packet/ConnectionPackets.hpp"

#include <utility>

namespace net::minecraft::client::gui::screen {

ConnectScreen::ConnectScreen(Minecraft* minecraft, std::string host, int port)
    : host_(std::move(host)),
      port_(port)
{
    if (minecraft == nullptr) {
        return;
    }

    minecraft->setWorld(nullptr);
    connectThread_ = std::thread([this, minecraft]() {
        if (connectingCancelled_.load(std::memory_order_acquire)) {
            return;
        }

        std::string connectError;
        auto bridge = std::make_unique<core::ClientNetworkBridge>(&minecraft->worldSession());
        if (!bridge->connect(minecraft, host_, port_, connectError)) {
            if (connectingCancelled_.load(std::memory_order_acquire)) {
                return;
            }
            std::lock_guard lock(connectMutex_);
            connectState_ = ConnectState::Failed;
            connectError_ = connectError.empty() ? "Failed to connect" : std::move(connectError);
            return;
        }

        if (connectingCancelled_.load(std::memory_order_acquire)) {
            bridge->disconnect();
            return;
        }

        if (net::minecraft::Connection* connection = bridge->connection()) {
            connection->sendPacket<HandshakePacket>(minecraft->session.username);
        }
        if (network::ClientNetworkHandler* handler = bridge->handler()) {
            handler->message = resource::language::I18n::getTranslation("connect.authorizing");
        }

        std::lock_guard lock(connectMutex_);
        pendingBridge_ = std::move(bridge);
        connectState_ = ConnectState::Connected;
    });
}

ConnectScreen::~ConnectScreen()
{
    connectingCancelled_.store(true, std::memory_order_release);
    if (connectThread_.joinable()) {
        connectThread_.join();
    }
}

void ConnectScreen::tick()
{
    {
        std::lock_guard lock(connectMutex_);
        if (connectState_ == ConnectState::Failed && minecraft() != nullptr) {
            minecraft()->setScreen(std::make_unique<DisconnectedScreen>(
                "connect.failed", "disconnect.genericReason", std::vector<std::string>{connectError_}));
            connectState_ = ConnectState::Handled;
            return;
        }
        // Hand the connected bridge to the persistent owner (WorldSession) so it outlives this
        // screen's retirement. The bridge owns the ClientNetworkHandler, which owns the live
        // ClientWorld; if it died with the ConnectScreen, client.world would dangle on the next
        // tick's raycast.
        if (pendingBridge_ != nullptr && minecraft() != nullptr) {
            minecraft()->worldSession().adoptNetworkBridge(std::move(pendingBridge_));
        }
    }

    if (connectingCancelled_.load(std::memory_order_acquire) || minecraft() == nullptr) {
        return;
    }
    if (core::ClientNetworkBridge* bridge = minecraft()->worldSession().networkBridge()) {
        bridge->tick();
    }
}

void ConnectScreen::render(int mouseX, int mouseY, float delta)
{
    renderBackground();
    if (textRenderer() != nullptr) {
        core::ClientNetworkBridge* bridge = nullptr;
        if (minecraft() != nullptr) {
            std::lock_guard lock(connectMutex_);
            bridge = pendingBridge_ != nullptr ? pendingBridge_.get()
                                               : minecraft()->worldSession().networkBridge();
        }
        const bool connected = bridge != nullptr;
        const std::string title = connected ? resource::language::I18n::getTranslation("connect.authorizing")
                                            : resource::language::I18n::getTranslation("connect.connecting");
        drawCenteredTextWithShadow(*textRenderer(), title, width_ / 2, height_ / 2 - 50, 0xFFFFFF);
        const std::string message =
            connected && bridge->handler() != nullptr ? bridge->handler()->message : "";
        drawCenteredTextWithShadow(*textRenderer(), message, width_ / 2, height_ / 2 - 10, 0xFFFFFF);
    }
    Screen::render(mouseX, mouseY, delta);
}

} // namespace net::minecraft::client::gui::screen
