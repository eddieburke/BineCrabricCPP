#pragma once

#include "net/minecraft/client/gui/screen/ChatScreen.hpp"

namespace net::minecraft::client::gui::screen {

class SleepingChatScreen : public ChatScreen {
public:
    void init() override;
    void removed() override;
    void keyPressed(char character, int keyCode) override;
    void keyPressed(int key) override;

private:
    void stopSleeping();
};

} // namespace net::minecraft::client::gui::screen
