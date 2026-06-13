#include "net/minecraft/client/gui/screen/option/WorldSettingsScreen.hpp"

#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/gui/layout/OptionsLayout.hpp"
#include "net/minecraft/client/option/OptionRegistry.hpp"

#include "net/minecraft/client/gui/screen/option/FogSettingsScreenFactory.hpp"

#include "net/minecraft/client/gui/screen/option/OptionGui.hpp"
#include "net/minecraft/client/option/OptionSpec.hpp"

#include "net/minecraft/client/resource/language/I18n.hpp"



#include <array>



namespace net::minecraft::client::gui::screen::option {

using client_option::GameOptions;

namespace world_screen {



namespace d = net::minecraft::client::option::option_spec_detail;

using net::minecraft::client::option::ApplyFlags;

using net::minecraft::client::option::GameOptions;

using net::minecraft::client::option::OptionSpec;



constexpr std::array<int, 4> kAutoSaveTicks {40, 400, 4000, 40000};



void cycleAutoSaveTicks(GameOptions& o, int delta)

{

    o.ofAutoSaveTicks = d::cycleDiscrete(o.ofAutoSaveTicks, delta, kAutoSaveTicks.data(),

        static_cast<int>(kAutoSaveTicks.size()));

}



std::array<OptionSpec, 4> kSpecs {{

    d::makeToggle("ofWeather", 44, ApplyFlags::ApplyToWorld,

        d::getBoolMember<&GameOptions::ofWeather>,

        d::cycleBoolMember<&GameOptions::ofWeather>,

        d::loadBoolMember<&GameOptions::ofWeather>,

        d::saveBoolMember<&GameOptions::ofWeather>),

    d::makeCycle("ofTime", 45, ApplyFlags::ApplyToWorld,

        d::cycleIntMod<&GameOptions::ofTime, 3>,

        d::loadIntMember<&GameOptions::ofTime>,

        d::saveIntMember<&GameOptions::ofTime>),

    d::makeCycle("ofAutoSaveTicks", 46, ApplyFlags::ApplyToWorld,

        cycleAutoSaveTicks,

        d::loadIntMember<&GameOptions::ofAutoSaveTicks>,

        d::saveIntMember<&GameOptions::ofAutoSaveTicks>),

    d::makeToggle("ofFastDebugInfo", 47, ApplyFlags::None,

        d::getBoolMember<&GameOptions::ofFastDebugInfo>,

        d::cycleBoolMember<&GameOptions::ofFastDebugInfo>,

        d::loadBoolMember<&GameOptions::ofFastDebugInfo>,

        d::saveBoolMember<&GameOptions::ofFastDebugInfo>),

}};



} // namespace world_screen



WorldSettingsScreen::WorldSettingsScreen(ParentFactory parentFactory, client_option::GameOptions* gameOptions)

    : parentFactory_(std::move(parentFactory)),

      gameOptions_(gameOptions),

      title_("World Settings")

{

}



void WorldSettingsScreen::init()

{

    buttons_.clear();

    if (gameOptions_ == nullptr || minecraft() == nullptr) {
        return;
    }

    client_option::OptionRegistry::registerAll();
    OptionGuiBuilder gui(*this, *minecraft(), *gameOptions_);
    const int x1 = gui.gridX(0);
    const int x2 = gui.gridX(1);
    const int y0 = gui.gridY(0);
    constexpr int dy = layout::kRowSpacing;



    gui.toggle(x1, y0, "ofWeather", "Weather");

    gui.intCycle(x2, y0, "ofTime", "Time", {"Default", "Day", "Night"}, &GameOptions::ofTime);

    gui.customCycle(x1, y0 + dy, "ofAutoSaveTicks",

        [](const GameOptions& o) {

            static constexpr std::array<const char*, 4> labels {"2s", "20s", "3min", "30min"};

            for (std::size_t i = 0; i < world_screen::kAutoSaveTicks.size(); ++i) {

                if (world_screen::kAutoSaveTicks[i] == o.ofAutoSaveTicks) {

                    return optionLabel("Autosave", labels[i]);

                }

            }

            return optionLabel("Autosave", "?");

        });

    gui.toggle(x2, y0 + dy, "ofFastDebugInfo", "Fast Debug Info");



    layout::refreshOptionStates(buttons_, *gameOptions_);



    const int navY = layout::optionsGridY(height(), 3);

    const auto returnHere = [factory = parentFactory_, options = gameOptions_]() {

        return std::make_unique<WorldSettingsScreen>(factory, options);

    };



    addCenteredActionButton(navY, 120, layout::kDefaultButtonHeight, "Fog Settings...",

        [this, returnHere] {

            if (gameOptions_ == nullptr || minecraft() == nullptr) {

                return;

            }

            gameOptions_->save();

            navigateTo([returnHere, opts = gameOptions_]() {

                return makeFogSettingsScreen(returnHere, opts);

            });

        });



    layout::OptionsBuildContext ctx { *this, *minecraft(), *gameOptions_ };

    layout::addDoneButton(ctx, navY + layout::kRowSpacing * 2,

        resource::language::I18n::getTranslation("gui.done"),

        [this] {

            if (gameOptions_ == nullptr || minecraft() == nullptr) {

                return;

            }

            gameOptions_->save();

            navigateTo(parentFactory_);

        });

}



void WorldSettingsScreen::render(int mouseX, int mouseY, float tickDelta)

{

    renderBackground();

    if (textRenderer() != nullptr) {

        drawCenteredTextWithShadow(*textRenderer(), title_, width() / 2, 20, 0xFFFFFF);

    }

    Screen::render(mouseX, mouseY, tickDelta);

}



} // namespace net::minecraft::client::gui::screen::option

