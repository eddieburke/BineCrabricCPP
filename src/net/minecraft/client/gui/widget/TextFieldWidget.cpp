#include "net/minecraft/client/gui/widget/TextFieldWidget.hpp"

#include "net/minecraft/client/font/TextRenderer.hpp"
#include "net/minecraft/client/gui/screen/Screen.hpp"
#include "net/minecraft/util/CharacterUtils.hpp"

namespace net::minecraft::client::gui::widget {
TextFieldWidget::TextFieldWidget(
    screen::Screen* parent, font::TextRenderer* textRenderer, int x, int y, int width, int height, std::string text)
    : parent_(parent), textRenderer_(textRenderer), x_(x), y_(y), width_(width), height_(height) {
    setText(std::move(text));
}

void TextFieldWidget::setText(const std::string& text) {
    text_ = text;
}

void TextFieldWidget::tick() {
    ++focusedTicks_;
}

void TextFieldWidget::keyPressed(char character, int keyCode) {
    if (!enabled || !focused || parent_ == nullptr) {
        return;
    }
    if (character == '\t') {
        parent_->handleTab();
    }
    bool paste = character == '\x16';
#ifdef _WIN32
    if (!paste && screen::Screen::pasteChordPressed(keyCode)) {
        paste = true;
    }
#endif
    if (paste) {
        std::string clipboard = screen::Screen::getClipboard();
        int room = 32 - static_cast<int>(text_.size());
        if (room > static_cast<int>(clipboard.size())) {
            room = static_cast<int>(clipboard.size());
        }
        if (room > 0) {
            text_ += clipboard.substr(0, static_cast<std::size_t>(room));
        }
    }
    if (screen::Screen::backspacePressed(keyCode) && !text_.empty()) {
        text_.pop_back();
    }
    const std::string& valid = CharacterUtils::validCharacters();
    if (character != 0 && valid.find(character) != std::string::npos &&
        (maxLength_ == 0 || static_cast<int>(text_.size()) < maxLength_)) {
        text_.push_back(character);
    }
}

void TextFieldWidget::mouseClicked(int mouseX, int mouseY, int button) {
    if (button != 0) {
        return;
    }
    const bool inside = enabled && mouseX >= x_ && mouseX < x_ + width_ && mouseY >= y_ && mouseY < y_ + height_;
    setFocused(inside);
}

void TextFieldWidget::setFocused(bool focused) {
    if (focused && !this->focused) {
        focusedTicks_ = 0;
    }
    this->focused = focused;
}

void TextFieldWidget::render() {
    if (textRenderer_ == nullptr) {
        return;
    }
    fill(x_ - 1, y_ - 1, x_ + width_ + 1, y_ + height_ + 1, 0xFF9F9F9FU);
    fill(x_, y_, x_ + width_, y_ + height_, 0xFF000000U);
    const int textY = y_ + (height_ - 8) / 2;
    if (enabled) {
        const bool blink = focused && (focusedTicks_ / 6) % 2 == 0;
        const std::string drawText = blink ? text_ + "_" : text_;
        drawTextWithShadow(*textRenderer_, drawText, x_ + 4, textY, 0xE0E0E0);
    } else {
        drawTextWithShadow(*textRenderer_, text_, x_ + 4, textY, 0x707070);
    }
}

void TextFieldWidget::setMaxLength(int maxLength) {
    maxLength_ = maxLength;
}

void TextFieldWidget::setBounds(int x, int y, int width, int height) {
    x_ = x;
    y_ = y;
    width_ = width;
    height_ = height;
}
}  // namespace net::minecraft::client::gui::widget
