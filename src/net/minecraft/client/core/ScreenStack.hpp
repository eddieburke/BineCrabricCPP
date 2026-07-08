#pragma once
#include <memory>
#include <vector>

namespace net::minecraft::client::gui::screen {
class Screen;
}

namespace net::minecraft::client {
class Minecraft;
}

namespace net::minecraft::client::core {
/// Owns the active GUI screen and deferred retirement queue (Java GC semantics).
/// Anchor: Minecraft.cpp L413–450, L522–523.
class ScreenStack {
   public:
    explicit ScreenStack(Minecraft& client) noexcept;
    void setScreen(std::unique_ptr<gui::screen::Screen> screen);
    /// Destroys screens parked by the previous frame's setScreen calls. Call once at run-loop top.
    void flushRetired();

    [[nodiscard]] gui::screen::Screen* current() const noexcept {
        return currentScreen_.get();
    }

    [[nodiscard]] bool hasScreen() const noexcept {
        return currentScreen_ != nullptr;
    }

    void tickScreens(Minecraft& client);

   private:
    void lockMouseWithoutClosingScreen();
    Minecraft& client_;
    std::unique_ptr<gui::screen::Screen> currentScreen_;
    std::vector<std::unique_ptr<gui::screen::Screen>> retiredScreens_;
};
}  // namespace net::minecraft::client::core
