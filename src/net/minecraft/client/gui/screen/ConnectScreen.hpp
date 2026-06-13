#pragma once



#include "net/minecraft/client/Minecraft.hpp"

#include "net/minecraft/client/core/ClientNetworkBridge.hpp"

#include "net/minecraft/client/gui/layout/ScreenLayout.hpp"

#include "net/minecraft/client/gui/screen/Screen.hpp"



#include <atomic>

#include <memory>

#include <mutex>

#include <string>

#include <thread>

#include <utility>



namespace net::minecraft::client::gui::screen {



class ConnectScreen : public Screen {

public:

    ConnectScreen() = default;

    ConnectScreen(Minecraft* minecraft, std::string host, int port);

    ~ConnectScreen() override;



    void tick() override;



    void init() override

    {

        buttons_.clear();

        addCenteredActionButton(layout::dialogFooterY(height_), "Cancel",

            [this] {

                if (minecraft() == nullptr) {

                    return;

                }

                connectingCancelled_.store(true, std::memory_order_release);

                core::ClientNetworkBridge* bridge = nullptr;

                {

                    std::lock_guard lock(connectMutex_);

                    bridge = pendingBridge_ != nullptr

                        ? pendingBridge_.get()

                        : minecraft()->worldSession().networkBridge();

                }

                if (bridge != nullptr) {

                    bridge->disconnect();

                }

                quitToTitle();

            });

    }



    void render(int mouseX, int mouseY, float delta) override;



private:

    enum class ConnectState {

        Connecting,

        Connected,

        Failed,

        Handled,

    };



    // Built on the connect thread; moved into Minecraft's WorldSession (the persistent owner)
    // by tick() on the main thread once connected. Null after that handoff.
    std::unique_ptr<core::ClientNetworkBridge> pendingBridge_ {};

    std::atomic<bool> connectingCancelled_ {false};

    std::mutex connectMutex_;

    ConnectState connectState_ = ConnectState::Connecting;

    std::string connectError_ {};

    std::thread connectThread_ {};

    std::string host_ {};

    int port_ = 0;

};



} // namespace net::minecraft::client::gui::screen

