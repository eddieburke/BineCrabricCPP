#pragma once

#include "net/minecraft/client/gui/DrawContext.hpp"
#include "net/minecraft/client/gui/widget/ActionButtonWidget.hpp"
#include "net/minecraft/client/gui/widget/ButtonWidget.hpp"

#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace net::minecraft::client {
class Minecraft;
}

namespace net::minecraft::client::font {
class TextRenderer;
}

namespace net::minecraft::client::gui::screen {

class Screen;

using ScreenFactory = std::function<std::unique_ptr<Screen>()>;

class Screen : public gui::DrawContext {
public:
    virtual ~Screen() = default;

    void init(client::Minecraft* minecraft, int width, int height);
    virtual void init() {}
    virtual void tick() {}

    virtual void render(int mouseX, int mouseY, float tickDelta);
    virtual void tickInput();
    virtual void onMouseEvent();
    virtual void onKeyboardEvent();
    virtual void keyPressed(char character, int keyCode);
    virtual void keyPressed(int key);

    [[nodiscard]] static std::string getClipboard();
    virtual void mouseClicked(int mouseX, int mouseY, int button);
    virtual void mouseReleased(int mouseX, int mouseY, int button);
    virtual void removed() {}
    [[nodiscard]] virtual bool shouldPause() const { return true; }

    void renderBackground();
    void renderBackground(int vOffset);
    void renderBackgroundTexture(int vOffset);

    virtual void confirmed(bool confirmed, int id);
    virtual void handleTab();

    [[nodiscard]] int width() const { return width_; }
    [[nodiscard]] int height() const { return height_; }
    [[nodiscard]] client::Minecraft* minecraft() const { return minecraft_; }
    [[nodiscard]] font::TextRenderer* textRenderer() const { return textRenderer_; }
    [[nodiscard]] std::vector<std::unique_ptr<widget::ButtonWidget>>& buttons() { return buttons_; }
    [[nodiscard]] const std::vector<std::unique_ptr<widget::ButtonWidget>>& buttons() const { return buttons_; }

    template<typename T, typename... Args>
    T& addButton(Args&&... args)
    {
        auto button = std::make_unique<T>(std::forward<Args>(args)...);
        T& ref = *button;
        buttons_.push_back(std::move(button));
        return ref;
    }

    widget::ActionButtonWidget& addActionButton(int x, int y, std::string text, std::function<void()> onClick);
    widget::ActionButtonWidget& addActionButton(int x, int y, int width, int height,
        std::string text, std::function<void()> onClick);
    widget::ActionButtonWidget& addActionButton(int id, int x, int y, int width, int height,
        std::string text, std::function<void()> onClick);
    widget::ActionButtonWidget& addCenteredActionButton(int y, std::string text, std::function<void()> onClick);
    widget::ActionButtonWidget& addCenteredActionButton(int y, int width, int height,
        std::string text, std::function<void()> onClick);

    bool passEvents = false;

protected:
    virtual void buttonClicked(widget::ButtonWidget& button);

    void dispatchButtonPress(widget::ButtonWidget& button);
    void closeScreen();
    void navigateTo(ScreenFactory factory);
    void navigateTo(std::unique_ptr<Screen> screen);
    void quitToTitle();
    void quitGame();

    client::Minecraft* minecraft_ = nullptr;
    font::TextRenderer* textRenderer_ = nullptr;
    int width_ = 0;
    int height_ = 0;
    std::vector<std::unique_ptr<widget::ButtonWidget>> buttons_ {};

private:
    widget::ButtonWidget* selectedButton_ = nullptr;
};

} // namespace net::minecraft::client::gui::screen
