#include "net/minecraft/client/gui/screen/option/SettingsScreen.hpp"

#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/gui/layout/OptionsLayout.hpp"
#include "net/minecraft/client/gui/screen/option/OptionGui.hpp"
#include "net/minecraft/client/option/OptionRegistry.hpp"
#include "net/minecraft/client/resource/language/I18n.hpp"

namespace net::minecraft::client::gui::screen::option {

SettingsScreen::SettingsScreen(ParentFactory parentFactory,
    client_option::GameOptions* gameOptions,
    std::string title)
    : parentFactory_(std::move(parentFactory)),
      gameOptions_(gameOptions),
      title_(std::move(title))
{
}

void SettingsScreen::init()
{
    buttons_.clear();
    if (gameOptions_ == nullptr || minecraft() == nullptr) {
        return;
    }

    client_option::OptionRegistry::registerAll();
    OptionGuiBuilder gui(*this, *minecraft(), *gameOptions_);
    buildOptions(gui);
    refreshOptionStates();

    layout::OptionsBuildContext ctx { *this, *minecraft(), *gameOptions_ };
    layout::addDoneButton(ctx, doneButtonY(), resource::language::I18n::getTranslation("gui.done"),
        [this] {
            if (gameOptions_ == nullptr || minecraft() == nullptr) {
                return;
            }
            gameOptions_->save();
            if (parentFactory_) {
                navigateTo(parentFactory_);
            }
        });
}

void SettingsScreen::render(int mouseX, int mouseY, float tickDelta)
{
    renderBackground();
    if (textRenderer() != nullptr) {
        drawCenteredTextWithShadow(*textRenderer(), title_, width() / 2, 20, 0xFFFFFF);
    }
    Screen::render(mouseX, mouseY, tickDelta);
}

void SettingsScreen::refreshOptionStates()
{
    if (gameOptions_ == nullptr) {
        return;
    }
    layout::refreshOptionStates(buttons_, *gameOptions_);
    refreshOptionLabels(*this, *gameOptions_);
}

} // namespace net::minecraft::client::gui::screen::option
