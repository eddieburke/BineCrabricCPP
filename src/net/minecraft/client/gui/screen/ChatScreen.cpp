#include "net/minecraft/client/gui/screen/ChatScreen.hpp"

#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/gui/hud/InGameHud.hpp"
#include "net/minecraft/client/network/MultiplayerClientPlayerEntity.hpp"
#include "net/minecraft/util/CharacterUtils.hpp"

#include "net/minecraft/client/input/InputSystem.hpp"

namespace net::minecraft::client::gui::screen {

void ChatScreen::init()
{
    text_.clear();
    input::InputSystem::instance().setKeyboardRepeat(true);
}

void ChatScreen::removed()
{
    input::InputSystem::instance().setKeyboardRepeat(false);
}

void ChatScreen::tick()
{
    ++focusedTicks_;
}

void ChatScreen::render(int mouseX, int mouseY, float tickDelta)
{
    (void)mouseX;
    (void)mouseY;
    (void)tickDelta;
    fill(2, height_ - 14, width_ - 2, height_ - 2, 0x80000000U);
    if (textRenderer() != nullptr) {
        const std::string cursor = (focusedTicks_ / 6 % 2 == 0) ? "_" : "";
        drawTextWithShadow(*textRenderer(), "> " + text_ + cursor, 4, height_ - 12, 0xE0E0E0);
    }
    Screen::render(mouseX, mouseY, tickDelta);
}

void ChatScreen::keyPressed(char character, int keyCode)
{
    (void)keyCode;
    if (character == '\0') {
        return;
    }
    const std::string& valid = CharacterUtils::validCharacters();
    if (valid.find(character) != std::string::npos && text_.length() < 100) {
        text_.push_back(character);
    }
}

void ChatScreen::keyPressed(int key)
{
    if (key == 1) {
        if (minecraft() != nullptr) {
            minecraft()->setScreen(nullptr);
        }
        return;
    }
    if (key == 28) {
        if (minecraft() != nullptr) {
            const std::string trimmed = [&]() {
                std::size_t start = text_.find_first_not_of(" \t");
                if (start == std::string::npos) {
                    return std::string {};
                }
                std::size_t end = text_.find_last_not_of(" \t");
                return text_.substr(start, end - start + 1);
            }();
            if (!trimmed.empty() && !minecraft()->isCommand(trimmed)) {
                if (auto* mpPlayer = dynamic_cast<network::MultiplayerClientPlayerEntity*>(minecraft()->player)) {
                    mpPlayer->sendChatMessage(trimmed);
                }
            }
            minecraft()->setScreen(nullptr);
        }
        return;
    }
    if (key == 14 && !text_.empty()) {
        text_.pop_back();
    }
}

void ChatScreen::mouseClicked(int mouseX, int mouseY, int button)
{
    if (button != 0 || minecraft() == nullptr) {
        Screen::mouseClicked(mouseX, mouseY, button);
        return;
    }
    if (!minecraft()->inGameHud.selectedName.empty()) {
        if (!text_.empty() && text_.back() != ' ') {
            text_.push_back(' ');
        }
        text_ += minecraft()->inGameHud.selectedName;
        if (text_.length() > 100) {
            text_.resize(100);
        }
        return;
    }
    Screen::mouseClicked(mouseX, mouseY, button);
}

} // namespace net::minecraft::client::gui::screen
