#pragma once

#include "net/minecraft/client/gui/screen/Screen.hpp"
#include "net/minecraft/client/gui/screen/option/OptionGui.hpp"
#include "net/minecraft/client/option/GameOptions.hpp"

#include <functional>
#include <memory>
#include <string>

namespace net::minecraft::client::gui::screen::option {

namespace client_option = net::minecraft::client::option;

class SettingsScreen : public screen::Screen {
public:
    using ParentFactory = std::function<std::unique_ptr<screen::Screen>()>;

    SettingsScreen(ParentFactory parentFactory, client_option::GameOptions* gameOptions, std::string title);

    void init() override;
    void render(int mouseX, int mouseY, float tickDelta) override;

protected:
    void buttonClicked(widget::ButtonWidget& button) override;
    virtual void buildOptions(OptionGuiBuilder& gui) = 0;
    virtual int doneButtonY() const { return height() / 6 + 24 * 8; }

    [[nodiscard]] client_option::GameOptions* gameOptions() const noexcept { return gameOptions_; }
    [[nodiscard]] const ParentFactory& parentFactory() const noexcept { return parentFactory_; }

private:
    void refreshOptionStates();

    ParentFactory parentFactory_;
    client_option::GameOptions* gameOptions_ = nullptr;
    std::string title_;
};

} // namespace net::minecraft::client::gui::screen::option
