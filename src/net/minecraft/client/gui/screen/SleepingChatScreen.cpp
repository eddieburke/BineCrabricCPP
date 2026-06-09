#include "net/minecraft/client/gui/screen/SleepingChatScreen.hpp"

#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/gui/layout/ScreenLayout.hpp"
#include "net/minecraft/client/network/MultiplayerClientPlayerEntity.hpp"
#include "net/minecraft/client/resource/language/I18n.hpp"
#include "net/minecraft/network/packet/PlayerPackets.hpp"

namespace net::minecraft::client::gui::screen {

void SleepingChatScreen::init()
{
    ChatScreen::init();
    addCenteredActionButton(height() - 40,
        resource::language::I18n::getTranslation("multiplayer.stopSleeping"),
        [this] { stopSleeping(); });
}

void SleepingChatScreen::removed()
{
    ChatScreen::removed();
}

void SleepingChatScreen::keyPressed(char character, int keyCode)
{
    (void)character;
    if (keyCode == 1) {
        stopSleeping();
        return;
    }
    if (keyCode == 28) {
        if (minecraft() != nullptr && minecraft()->player != nullptr) {
            std::string trimmed = text_;
            const std::size_t start = trimmed.find_first_not_of(" \t");
            if (start != std::string::npos) {
                const std::size_t end = trimmed.find_last_not_of(" \t");
                trimmed = trimmed.substr(start, end - start + 1);
                if (!trimmed.empty()) {
                    if (auto* mpPlayer = dynamic_cast<network::MultiplayerClientPlayerEntity*>(minecraft()->player)) {
                        mpPlayer->sendChatMessage(trimmed);
                    }
                }
            }
            text_.clear();
        }
        return;
    }
    ChatScreen::keyPressed(character, keyCode);
}

void SleepingChatScreen::keyPressed(int key)
{
    keyPressed('\0', key);
}

void SleepingChatScreen::stopSleeping()
{
    if (minecraft() == nullptr || minecraft()->player == nullptr) {
        return;
    }
    if (auto* mpPlayer = dynamic_cast<network::MultiplayerClientPlayerEntity*>(minecraft()->player)) {
        if (mpPlayer->networkHandler != nullptr) {
            constexpr int kStopSleeping = 3;
            ClientCommandC2SPacket packet;
            packet.entityId = mpPlayer->id;
            packet.mode = kStopSleeping;
            mpPlayer->networkHandler->sendPacket(packet);
        }
    }
}

} // namespace net::minecraft::client::gui::screen
