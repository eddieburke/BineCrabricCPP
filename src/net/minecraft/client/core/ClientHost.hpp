#pragma once

// Port for Screen::init and ScreenStack lifecycle (M5).
// Minecraft (later GameApplication) implements this interface; Screen.hpp will migrate
// from Minecraft* to ClientHost& over time.

namespace net::minecraft {
class World;
}

namespace net::minecraft::client {
class Minecraft;
}

namespace net::minecraft::client::option {
class GameOptions;
}

namespace net::minecraft::entity::player {
class ClientPlayerEntity;
}

namespace net::minecraft::stat {
class PlayerStats;
}

namespace net::minecraft::client::core {

/// Minimal host surface for GUI screens and deferred screen retirement.
class ClientHost {
public:
    virtual ~ClientHost() = default;

    /// Transition shim until Screen::init(ClientHost&, …) replaces Minecraft*.
    [[nodiscard]] virtual Minecraft* asMinecraft() noexcept = 0;
    [[nodiscard]] virtual const Minecraft* asMinecraft() const noexcept = 0;

    [[nodiscard]] virtual World* world() const noexcept = 0;
    [[nodiscard]] virtual entity::player::ClientPlayerEntity* player() const noexcept = 0;
    [[nodiscard]] virtual stat::PlayerStats* stats() noexcept = 0;

    virtual void clearInGameChat() = 0;
    virtual void unlockMouse() = 0;
    /// Must route through ScreenStack::setScreen (see INTEGRATION_NOTES.md).
    virtual void lockMouse() = 0;

    [[nodiscard]] virtual option::GameOptions& options() noexcept = 0;
    [[nodiscard]] virtual int displayWidth() const noexcept = 0;
    [[nodiscard]] virtual int displayHeight() const noexcept = 0;
    virtual bool& skipGameRenderFlag() noexcept = 0;
};

} // namespace net::minecraft::client::core
