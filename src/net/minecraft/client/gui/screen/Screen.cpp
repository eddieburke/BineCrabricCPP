#include "net/minecraft/client/gui/screen/Screen.hpp"

#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/font/TextRenderer.hpp"
#include "net/minecraft/client/gui/layout/ScreenLayout.hpp"
#include "net/minecraft/client/gui/screen/TitleScreen.hpp"
#include "net/minecraft/client/gui/widget/ActionButtonWidget.hpp"
#include "net/minecraft/client/render/platform/GuiGlState.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/client/texture/TextureManager.hpp"
#include "net/minecraft/client/util/UiScale.hpp"

#include <utility>

#include "net/minecraft/client/input/InputSystem.hpp"

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace net::minecraft::client::gui::screen {

void Screen::init(client::Minecraft* minecraft, int width, int height)
{
    minecraft_ = minecraft;
    textRenderer_ = minecraft != nullptr ? minecraft->textRenderer.get() : nullptr;
    width_ = width;
    height_ = height;
    selectedButton_ = nullptr;
    buttons_.clear();
    init();
}

void Screen::render(int mouseX, int mouseY, float tickDelta)
{
    (void)tickDelta;
    if (minecraft_ == nullptr || textRenderer_ == nullptr) {
        return;
    }
    for (const std::unique_ptr<widget::ButtonWidget>& button : buttons_) {
        if (button == nullptr || !button->visible) {
            continue;
        }
        button->render(*minecraft_, *textRenderer_, mouseX, mouseY);
    }
}

void Screen::tickInput()
{
    input::InputSystem::instance().drainScreenEvents(*this);
}

void Screen::onMouseEvent()
{
    if (minecraft_ == nullptr) {
        return;
    }
    input::InputSystem& input = input::InputSystem::instance();
    const int dw = minecraft_->displayWidth;
    const int dh = minecraft_->displayHeight;
    if (dw <= 0 || dh <= 0) {
        return;
    }
    if (input.eventMouseWheel() != 0) {
        return;
    }
    const int button = input.eventMouseButton();
    if (button < 0) {
        return;
    }
    const auto [mouseX, mouseY] = util::mapScreenMouse(
        minecraft_->options,
        dw,
        dh,
        width_,
        height_,
        input.eventMouseX(),
        input.eventMouseY());
    if (input.eventMouseButtonDown()) {
        mouseClicked(mouseX, mouseY, button);
    } else {
        mouseReleased(mouseX, mouseY, button);
    }
}

void Screen::onKeyboardEvent()
{
    input::InputSystem& input = input::InputSystem::instance();
    if (!input.eventKeyDown()) {
        return;
    }
    if (input.eventKey() == 87 && minecraft_ != nullptr) {
        minecraft_->toggleFullscreen();
        return;
    }
    keyPressed(input.eventChar(), input.eventKey());
}

void Screen::keyPressed(char character, int keyCode)
{
    (void)character;
    keyPressed(keyCode);
}

void Screen::keyPressed(int key)
{
    if (key == 1 && minecraft_ != nullptr) {
        minecraft_->setScreen(nullptr);
    }
}

void Screen::mouseClicked(int mouseX, int mouseY, int button)
{
    if (button != 0) {
        return;
    }
    selectedButton_ = nullptr;
    for (const std::unique_ptr<widget::ButtonWidget>& widget : buttons_) {
        if (widget == nullptr || !widget->isMouseOver(mouseX, mouseY)) {
            continue;
        }
        selectedButton_ = widget.get();
        minecraft_->audio.play("random.click", 1.0f, 1.0f);
        dispatchButtonPress(*widget);
        break;
    }
}

widget::ActionButtonWidget& Screen::addActionButton(int x, int y, std::string text, std::function<void()> onClick)
{
    return addActionButton(x, y, layout::kDefaultButtonWidth, layout::kDefaultButtonHeight,
        std::move(text), std::move(onClick));
}

widget::ActionButtonWidget& Screen::addActionButton(int x, int y, int width, int height,
    std::string text, std::function<void()> onClick)
{
    return addActionButton(widget::ActionButtonWidget::kNoId, x, y, width, height,
        std::move(text), std::move(onClick));
}

widget::ActionButtonWidget& Screen::addActionButton(int id, int x, int y, int width, int height,
    std::string text, std::function<void()> onClick)
{
    auto button = std::make_unique<widget::ActionButtonWidget>(id, x, y, width, height,
        std::move(text), std::move(onClick));
    widget::ActionButtonWidget& ref = *button;
    buttons_.push_back(std::move(button));
    return ref;
}

widget::ActionButtonWidget& Screen::addCenteredActionButton(int y, std::string text, std::function<void()> onClick)
{
    return addCenteredActionButton(y, layout::kDefaultButtonWidth, layout::kDefaultButtonHeight,
        std::move(text), std::move(onClick));
}

widget::ActionButtonWidget& Screen::addCenteredActionButton(int y, int width, int height,
    std::string text, std::function<void()> onClick)
{
    return addActionButton(layout::centerBtnX(width_), y, width, height, std::move(text), std::move(onClick));
}

void Screen::dispatchButtonPress(widget::ButtonWidget& button)
{
    if (auto* actionButton = dynamic_cast<widget::ActionButtonWidget*>(&button)) {
        if (actionButton->onClick) {
            actionButton->onClick();
            return;
        }
    }
    buttonClicked(button);
}

void Screen::closeScreen()
{
    if (minecraft_ != nullptr) {
        minecraft_->setScreen(nullptr);
    }
}

void Screen::navigateTo(ScreenFactory factory)
{
    if (minecraft_ != nullptr && factory) {
        minecraft_->setScreen(factory());
    }
}

void Screen::navigateTo(std::unique_ptr<Screen> screen)
{
    if (minecraft_ != nullptr) {
        minecraft_->setScreen(std::move(screen));
    }
}

void Screen::quitToTitle()
{
    if (minecraft_ != nullptr) {
        minecraft_->setWorld(nullptr);
        minecraft_->setScreen(std::make_unique<TitleScreen>());
    }
}

void Screen::quitGame()
{
    if (minecraft_ != nullptr) {
        minecraft_->scheduleStop();
    }
}

void Screen::mouseReleased(int mouseX, int mouseY, int button)
{
    if (selectedButton_ != nullptr && button == 0) {
        selectedButton_->mouseReleased(mouseX, mouseY);
        selectedButton_ = nullptr;
    }
}

void Screen::buttonClicked(widget::ButtonWidget& button)
{
    (void)button;
}

void Screen::renderBackground()
{
    renderBackground(0);
}

void Screen::renderBackground(int vOffset)
{
    if (minecraft_ != nullptr && minecraft_->world != nullptr) {
        fillGradient(0, 0, width_, height_, 0xC0101010U, 0xD0101010U);
    } else {
        renderBackgroundTexture(vOffset);
    }
}

void Screen::renderBackgroundTexture(int vOffset)
{
    if (minecraft_ == nullptr) {
        fillGradient(0, 0, width_, height_, 0xFF101020U, 0xFF202040U);
        return;
    }

    render::Tessellator& tessellator = render::INSTANCE;
    const int textureId = minecraft_->textureManager.getTextureId("/gui/background.png");
    gl::GL11::glBindTexture(gl::GL11::GL_TEXTURE_2D, textureId);
    render::platform::GuiGlState::disableWorldEffects();
    render::platform::GuiGlState::resetColor();
    constexpr float tile = 32.0f;
    tessellator.startQuads();
    tessellator.color(0x404040);
    tessellator.vertex(0.0, static_cast<double>(height_), 0.0, 0.0,
        static_cast<double>(height_) / tile + static_cast<double>(vOffset));
    tessellator.vertex(static_cast<double>(width_), static_cast<double>(height_), 0.0,
        static_cast<double>(width_) / tile, static_cast<double>(height_) / tile + static_cast<double>(vOffset));
    tessellator.vertex(static_cast<double>(width_), 0.0, 0.0, static_cast<double>(width_) / tile,
        static_cast<double>(vOffset));
    tessellator.vertex(0.0, 0.0, 0.0, 0.0, static_cast<double>(vOffset));
    tessellator.draw();
}

void Screen::confirmed(bool confirmed, int id)
{
    (void)confirmed;
    (void)id;
}

void Screen::handleTab()
{
}

std::string Screen::getClipboard()
{
#ifdef _WIN32
    if (!OpenClipboard(nullptr)) {
        return {};
    }
    HANDLE data = GetClipboardData(CF_UNICODETEXT);
    if (data == nullptr) {
        CloseClipboard();
        return {};
    }
    const auto* wide = static_cast<const wchar_t*>(GlobalLock(data));
    if (wide == nullptr) {
        CloseClipboard();
        return {};
    }
    std::string result;
    for (const wchar_t* p = wide; *p != L'\0'; ++p) {
        const wchar_t ch = *p;
        if (ch <= 0x7F) {
            result.push_back(static_cast<char>(ch));
        }
    }
    GlobalUnlock(data);
    CloseClipboard();
    return result;
#else
    return {};
#endif
}

} // namespace net::minecraft::client::gui::screen
