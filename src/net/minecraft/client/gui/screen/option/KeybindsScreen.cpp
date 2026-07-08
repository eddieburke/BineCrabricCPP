#include "net/minecraft/client/gui/screen/option/KeybindsScreen.hpp"

#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/gui/layout/OptionsLayout.hpp"
#include "net/minecraft/client/gui/layout/ScreenLayout.hpp"
#include "net/minecraft/client/resource/language/I18n.hpp"

namespace net::minecraft::client::gui::screen::option {
KeybindsScreen::KeybindsScreen(ParentFactory parentFactory, client_option::GameOptions* gameOptions)
    : parentFactory_(std::move(parentFactory)), gameOptions_(gameOptions) {
}

void KeybindsScreen::init() {
    title_ = resource::language::I18n::getTranslation("controls.title");
    buttons_.clear();
    if (gameOptions_ == nullptr) {
        return;
    }
    const int listX = controlsListX();
    for (int i = 0; i < client_option::GameOptions::kKeybindCount; ++i) {
        const int index = i;
        addActionButton(listX + (i % 2) * 160,
                        height() / 6 + layout::kRowSpacing * (i / 2),
                        70,
                        20,
                        gameOptions_->getKeybindKey(i),
                        [this, index] { selectKeybind(index); });
    }
    addActionButton(
        layout::centerBtnX(width()), height() / 6 + 168, resource::language::I18n::getTranslation("gui.done"), [this] {
            if (parentFactory_) {
                navigateTo(parentFactory_);
            }
        });
}

void KeybindsScreen::render(int mouseX, int mouseY, float tickDelta) {
    renderBackground();
    if (textRenderer() != nullptr) {
        drawCenteredTextWithShadow(*textRenderer(), title_, width() / 2, 20, 0xFFFFFF);
        if (gameOptions_ != nullptr) {
            const int listX = controlsListX();
            for (int i = 0; i < client_option::GameOptions::kKeybindCount; ++i) {
                drawTextWithShadow(*textRenderer(),
                                   gameOptions_->getKeybindName(i),
                                   listX + (i % 2) * 160 + 70 + 6,
                                   height() / 6 + layout::kRowSpacing * (i / 2) + 7,
                                   0xFFFFFFFF);
            }
        }
    }
    Screen::render(mouseX, mouseY, tickDelta);
}

void KeybindsScreen::selectKeybind(int index) {
    if (gameOptions_ == nullptr || index < 0 || index >= client_option::GameOptions::kKeybindCount) {
        return;
    }
    for (int i = 0; i < client_option::GameOptions::kKeybindCount; ++i) {
        if (buttons_[static_cast<std::size_t>(i)] != nullptr) {
            buttons_[static_cast<std::size_t>(i)]->text = gameOptions_->getKeybindKey(i);
        }
    }
    selectedKeyBinding_ = index;
    buttons_[static_cast<std::size_t>(index)]->text = "> " + gameOptions_->getKeybindKey(index) + " <";
}

void KeybindsScreen::keyPressed(char character, int keyCode) {
    (void) character;
    if (selectedKeyBinding_ < 0 || gameOptions_ == nullptr) {
        Screen::keyPressed(character, keyCode);
        return;
    }
    gameOptions_->setKeybindKey(selectedKeyBinding_, keyCode);
    if (selectedKeyBinding_ >= 0 && selectedKeyBinding_ < client_option::GameOptions::kKeybindCount &&
        buttons_[static_cast<std::size_t>(selectedKeyBinding_)] != nullptr) {
        buttons_[static_cast<std::size_t>(selectedKeyBinding_)]->text =
            gameOptions_->getKeybindKey(selectedKeyBinding_);
    }
    selectedKeyBinding_ = -1;
}
}  // namespace net::minecraft::client::gui::screen::option
