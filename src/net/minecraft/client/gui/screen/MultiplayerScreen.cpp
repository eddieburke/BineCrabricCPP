#include "net/minecraft/client/gui/screen/MultiplayerScreen.hpp"

#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/gui/layout/ScreenLayout.hpp"
#include "net/minecraft/client/gui/screen/ConnectScreen.hpp"
#include "net/minecraft/client/gui/screen/TitleScreen.hpp"
#include "net/minecraft/client/resource/language/I18n.hpp"

#include "net/minecraft/client/input/InputSystem.hpp"

#include <algorithm>
#include <sstream>
#include <vector>

namespace net::minecraft::client::gui::screen {

MultiplayerScreen::MultiplayerScreen(ScreenFactory parentFactory) : parentFactory_(std::move(parentFactory))
{
    if (!parentFactory_) {
        parentFactory_ = []() { return std::make_unique<TitleScreen>(); };
    }
}

void MultiplayerScreen::init()
{
    input::InputSystem::instance().setKeyboardRepeat(true);
    buttons_.clear();
    connectButton_ = &addActionButton(layout::centerBtnX(width()), layout::formPrimaryBtnY(height()),
        resource::language::I18n::getTranslation("multiplayer.connect"),
        [this] { connectToServer(); });
    addActionButton(layout::centerBtnX(width()), layout::formCancelBtnY(height()),
        resource::language::I18n::getTranslation("gui.cancel"),
        [this] { navigateTo(parentFactory_); });

    std::string initialServer;
    if (minecraft() != nullptr) {
        initialServer = minecraft()->options.lastServer;
        for (char& ch : initialServer) {
            if (ch == '_') {
                ch = ':';
            }
        }
    }
    if (textRenderer() != nullptr) {
        serverField_ = std::make_unique<widget::TextFieldWidget>(
            this, textRenderer(), width() / 2 - 100, height() / 4 - 10 + 50 + 18, 200, 20, initialServer);
        serverField_->focused = true;
        serverField_->setMaxLength(128);
    }
    updateConnectButtonState();
}

void MultiplayerScreen::tick()
{
    if (serverField_ != nullptr) {
        serverField_->tick();
        updateConnectButtonState();
    }
}

void MultiplayerScreen::removed()
{
    input::InputSystem::instance().setKeyboardRepeat(false);
}

int MultiplayerScreen::parseInt(const std::string& value, int defaultValue)
{
    try {
        return std::stoi(value);
    } catch (...) {
        return defaultValue;
    }
}

void MultiplayerScreen::updateConnectButtonState()
{
    if (connectButton_ != nullptr) {
        connectButton_->active = serverField_ != nullptr && !serverField_->getText().empty();
    }
}

void MultiplayerScreen::connectToServer()
{
    if (connectButton_ == nullptr || !connectButton_->active || minecraft() == nullptr || serverField_ == nullptr) {
        return;
    }
    std::string address = serverField_->getText();
    const auto trimLeft = address.find_first_not_of(" \t\r\n");
    if (trimLeft != std::string::npos) {
        const auto trimRight = address.find_last_not_of(" \t\r\n");
        address = address.substr(trimLeft, trimRight - trimLeft + 1);
    } else {
        address.clear();
    }
    std::string stored = address;
    std::replace(stored.begin(), stored.end(), ':', '_');
    minecraft()->options.lastServer = stored;
    minecraft()->options.save();

    std::vector<std::string> parts;
    if (!address.empty() && address.front() == '[') {
        const std::size_t close = address.find(']');
        if (close != std::string::npos) {
            parts.push_back(address.substr(1, close - 1));
            std::string remainder = address.substr(close + 1);
            const auto trim = remainder.find_first_not_of(" \t");
            if (trim != std::string::npos) {
                remainder = remainder.substr(trim);
            }
            if (!remainder.empty() && remainder.front() == ':') {
                remainder = remainder.substr(1);
            }
            if (!remainder.empty()) {
                parts.push_back(remainder);
            }
        } else {
            parts.push_back(address);
        }
    } else {
        std::istringstream stream(address);
        std::string part;
        while (std::getline(stream, part, ':')) {
            parts.push_back(part);
        }
    }
    if (parts.size() > 2) {
        parts = {address};
    }
    const std::string host = parts.empty() ? address : parts.front();
    const int port = parts.size() > 1 ? parseInt(parts[1], 25565) : 25565;
    navigateTo(std::make_unique<ConnectScreen>(minecraft(), host, port));
}

void MultiplayerScreen::keyPressed(char character, int keyCode)
{
    if (serverField_ != nullptr) {
        serverField_->keyPressed(character, keyCode);
    }
    updateConnectButtonState();
    if (character == '\r') {
        connectToServer();
    }
}

void MultiplayerScreen::mouseClicked(int mouseX, int mouseY, int button)
{
    Screen::mouseClicked(mouseX, mouseY, button);
    if (serverField_ != nullptr) {
        serverField_->mouseClicked(mouseX, mouseY, button);
    }
}

void MultiplayerScreen::render(int mouseX, int mouseY, float tickDelta)
{
    renderBackground();
    if (textRenderer() != nullptr) {
        drawCenteredTextWithShadow(*textRenderer(),
            resource::language::I18n::getTranslation("multiplayer.title"),
            width() / 2,
            layout::formTitleY(height()),
            0xFFFFFF);
        drawTextWithShadow(*textRenderer(),
            resource::language::I18n::getTranslation("multiplayer.info1"),
            layout::formBodyLeftX(width()),
            height() / 4 - 60 + 60 + 0,
            0xA0A0A0);
        drawTextWithShadow(*textRenderer(),
            resource::language::I18n::getTranslation("multiplayer.info2"),
            layout::formBodyLeftX(width()),
            height() / 4 - 60 + 60 + 9,
            0xA0A0A0);
        drawTextWithShadow(*textRenderer(),
            resource::language::I18n::getTranslation("multiplayer.ipinfo"),
            width() / 2 - 100,
            height() / 4 - 10 + 50,
            0xA0A0A0);
    }
    if (serverField_ != nullptr) {
        serverField_->render();
    }
    Screen::render(mouseX, mouseY, tickDelta);
}

} // namespace net::minecraft::client::gui::screen
