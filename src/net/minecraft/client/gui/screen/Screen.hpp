#pragma once
#include <functional>
#include <initializer_list>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "net/minecraft/client/gui/DrawContext.hpp"
#include "net/minecraft/client/gui/widget/ActionButtonWidget.hpp"
#include "net/minecraft/client/gui/widget/ButtonWidget.hpp"
#include "net/minecraft/client/input/Keys.hpp"
#include "net/minecraft/mod/ScreenUi.hpp"

namespace net::minecraft::client {
class Minecraft;
}

namespace net::minecraft::client::font {
class TextRenderer;
}

namespace net::minecraft::client::gui::widget {
class TextFieldWidget;
}

namespace net::minecraft::client::gui::screen {
class Screen;
using ScreenFactory = std::function<std::unique_ptr<Screen>()>;

class Screen : public gui::DrawContext {
   public:
    virtual ~Screen() = default;
    void init(client::Minecraft* minecraft, int width, int height);

    virtual void init() {
    }

    [[nodiscard]] virtual std::string_view getScreenUiId() const {
        return {};
    }

    void publishScreenUi(std::string_view region, int* stackedButtonY = nullptr);

    virtual void tick() {
    }

    virtual void render(int mouseX, int mouseY, float tickDelta);
    virtual void tickInput();
    virtual void onMouseEvent();
    virtual void onKeyboardEvent();
    virtual void keyPressed(char character, int keyCode);

    [[nodiscard]] static bool escapePressed(int keyCode) noexcept {
        return keyCode == input::keys::kEscape;
    }

    [[nodiscard]] static bool submitPressed(int keyCode, char character) noexcept {
        return keyCode == input::keys::kEnter || character == '\r';
    }

    [[nodiscard]] static bool backspacePressed(int keyCode) noexcept {
        return keyCode == input::keys::kBackspace;
    }

    [[nodiscard]] static bool arrowUpPressed(int keyCode) noexcept {
        return keyCode == input::keys::kUp;
    }

    [[nodiscard]] static bool arrowDownPressed(int keyCode) noexcept {
        return keyCode == input::keys::kDown;
    }

    [[nodiscard]] static bool pasteChordPressed(int keyCode) noexcept;
    /// Default Escape behavior — closes this screen. Returns true if handled.
    bool closeOnEscape(int keyCode);
    [[nodiscard]] static std::string getClipboard();
    virtual void mouseClicked(int mouseX, int mouseY, int button);
    virtual void mouseReleased(int mouseX, int mouseY, int button);

    /// Mouse wheel notification. delta is the raw wheel value (sign = direction).
    virtual void mouseScrolled(int mouseX, int mouseY, int delta) {
        (void) mouseX;
        (void) mouseY;
        (void) delta;
    }

    virtual void removed() {
    }

    /// Named text fields exposed to Lua mods on host (vanilla) screens.
    [[nodiscard]] virtual std::string hostFieldText(std::string_view name) const {
        (void) name;
        return {};
    }

    virtual bool setHostFieldText(std::string_view name, std::string value) {
        (void) name;
        (void) value;
        return false;
    }

    virtual void forEachHostField(const std::function<void(std::string_view name, std::string_view value)>& fn) const {
        (void) fn;
    }

    [[nodiscard]] virtual bool shouldPause() const {
        return true;
    }

    void renderBackground();
    void renderBackground(int vOffset);
    void renderBackgroundTexture(int vOffset);
    virtual void confirmed(bool confirmed, int id);
    virtual void handleTab();

    [[nodiscard]] int width() const {
        return width_;
    }

    [[nodiscard]] int height() const {
        return height_;
    }

    [[nodiscard]] client::Minecraft* minecraft() const {
        return minecraft_;
    }

    [[nodiscard]] font::TextRenderer* textRenderer() const {
        return textRenderer_;
    }

    [[nodiscard]] std::vector<std::unique_ptr<widget::ButtonWidget>>& buttons() {
        return buttons_;
    }

    [[nodiscard]] const std::vector<std::unique_ptr<widget::ButtonWidget>>& buttons() const {
        return buttons_;
    }

    template <typename T, typename... Args>
    T& addButton(Args&&... args) {
        auto button = std::make_unique<T>(std::forward<Args>(args)...);
        T& ref = *button;
        buttons_.push_back(std::move(button));
        return ref;
    }

    widget::ActionButtonWidget& addActionButton(int x, int y, std::string text, std::function<void()> onClick);
    widget::ActionButtonWidget& addActionButton(
        int x, int y, int width, int height, std::string text, std::function<void()> onClick);
    widget::ActionButtonWidget& addActionButton(
        int id, int x, int y, int width, int height, std::string text, std::function<void()> onClick);
    widget::ActionButtonWidget& addCenteredActionButton(int y, std::string text, std::function<void()> onClick);
    widget::ActionButtonWidget& addCenteredActionButton(
        int y, int width, int height, std::string text, std::function<void()> onClick);
    bool passEvents = false;

   protected:
    void enableTextInput();
    void disableTextInput();
    void routeKeyToTextFields(char character, int keyCode, std::initializer_list<widget::TextFieldWidget*> fields);
    void clickTextFields(int mouseX, int mouseY, int button, std::initializer_list<widget::TextFieldWidget*> fields);
    void tickTextFields(std::initializer_list<widget::TextFieldWidget*> fields);
    void handleFormKeyPress(char character,
                            int keyCode,
                            std::initializer_list<widget::TextFieldWidget*> fields,
                            const std::function<void()>& onSubmit);
    void closeScreen();
    void navigateTo(ScreenFactory factory);
    void navigateTo(std::unique_ptr<Screen> screen);
    void quitToTitle();
    void quitGame();
    client::Minecraft* minecraft_ = nullptr;
    font::TextRenderer* textRenderer_ = nullptr;
    int width_ = 0;
    int height_ = 0;
    std::vector<std::unique_ptr<widget::ButtonWidget>> buttons_{};

   private:
    void dispatchButtonPress(widget::ButtonWidget& button);
    widget::ButtonWidget* selectedButton_ = nullptr;
};
}  // namespace net::minecraft::client::gui::screen
