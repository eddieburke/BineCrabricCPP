#include <memory>
#include <string>
#include <vector>
#include <gtest/gtest.h>

#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/gui/layout/OptionsListScroll.hpp"
#include "net/minecraft/client/gui/screen/option/KeybindsScreen.hpp"
#include "net/minecraft/client/gui/screen/option/OptionGui.hpp"
#include "net/minecraft/client/gui/screen/option/VideoOptionsScreen.hpp"
#include "net/minecraft/client/gui/widget/ActionButtonWidget.hpp"
#include "net/minecraft/client/gui/widget/ButtonWidget.hpp"
#include "net/minecraft/client/gui/widget/OptionButtonWidget.hpp"
#include "net/minecraft/client/gui/widget/SliderWidget.hpp"
#include "net/minecraft/client/gui/widget/TextFieldWidget.hpp"
#include "net/minecraft/client/input/InputSystem.hpp"
#include "net/minecraft/client/input/Keys.hpp"
#include "net/minecraft/client/platform/glfw/Window.hpp"
#include "net/minecraft/client/resource/language/I18n.hpp"
#include "net/minecraft/util/CharacterUtils.hpp"

namespace net::minecraft::test {
namespace {
using client::gui::layout::OptionsListScroll;
using client::gui::widget::ButtonWidget;

class TestMinecraft final : public client::Minecraft {
 public:
 using client::Minecraft::Minecraft;
 void handleCrash(const util::crash::CrashReport&) override {}
};

class EmptyScreen final : public client::gui::screen::Screen {};

class CountingScreen final : public client::gui::screen::Screen {
 public:
 int mouseEvents = 0;
 int keyboardEvents = 0;
 void onMouseEvent(const client::input::MouseEvent&) override {
  ++mouseEvents;
 }
 void onKeyboardEvent(const client::input::KeyboardEvent&) override {
  ++keyboardEvents;
 }
};

class ActiveWindowGuard {
 public:
 ActiveWindowGuard() : wasActive_(client::platform::glfw::Window::isActive()) {
  client::platform::glfw::Window::setActive(true);
 }
 ~ActiveWindowGuard() {
  client::platform::glfw::Window::setActive(wasActive_);
 }

 private:
 bool wasActive_;
};

class NavigationScreen final : public client::gui::screen::Screen {
 public:
 int mouseEvents = 0;
 int keyboardEvents = 0;
 void onMouseEvent(const client::input::MouseEvent&) override {
  ++mouseEvents;
  if(minecraft() != nullptr) minecraft()->setScreen(std::make_unique<CountingScreen>());
 }
 void onKeyboardEvent(const client::input::KeyboardEvent&) override {
  ++keyboardEvents;
 }
};

TEST(GuiRegression, ScrollRebuildRestoresAndClampsOffset) {
 OptionsListScroll scroll;
 scroll.setViewport(32, 132);
 scroll.setContentHeight(332);
 scroll.setScrollOffset(180);
 scroll.clear();
 scroll.setViewport(40, 140);

 std::vector<std::unique_ptr<ButtonWidget>> buttons;
 buttons.push_back(std::make_unique<ButtonWidget>(0, 10, 40, 150, 20, "first"));
 buttons.push_back(std::make_unique<ButtonWidget>(1, 10, 220, 150, 20, "last"));
 scroll.captureButtons(buttons, buttons.size());

 EXPECT_EQ(scroll.scrollOffset(), 100);
 scroll.apply(buttons);
 EXPECT_FALSE(buttons[0]->visible);
 EXPECT_TRUE(buttons[1]->visible);
}

TEST(GuiRegression, ScrollHidesRowsThatWouldOverlapFooter) {
 OptionsListScroll scroll;
 scroll.setViewport(32, 70);
 std::vector<std::unique_ptr<ButtonWidget>> buttons;
 buttons.push_back(std::make_unique<ButtonWidget>(0, 10, 32, 150, 20, "inside"));
 buttons.push_back(std::make_unique<ButtonWidget>(1, 10, 56, 150, 20, "partial"));
 scroll.captureButtons(buttons, buttons.size());
 scroll.apply(buttons);

 EXPECT_TRUE(buttons[0]->visible);
 EXPECT_FALSE(buttons[1]->visible);
 EXPECT_FALSE(buttons[1]->isMouseOver(20, 60));
}

TEST(GuiRegression, NarrowWindowScrollbarCanBeDragged) {
 OptionsListScroll scroll;
 scroll.setViewport(32, 132);
 scroll.setContentHeight(332);
 ASSERT_TRUE(scroll.mouseClicked(314, 32, 320));
 scroll.mouseDragged(90);
 EXPECT_GT(scroll.scrollOffset(), 0);
 scroll.mouseReleased();
 EXPECT_FALSE(scroll.draggingThumb());
}

TEST(GuiRegression, TextFieldInsertionUsesConfiguredLimitAndCharacterFilter) {
 const std::string& valid = CharacterUtils::validCharacters();
 ASSERT_FALSE(valid.empty());
 client::gui::widget::TextFieldWidget field(nullptr, nullptr, 0, 0, 100, 20, "");
 field.setMaxLength(40);
 field.focused = true;
 field.writeText(std::string(41, valid.front()));
 EXPECT_EQ(field.getText().size(), 40U);
 field.setText(std::string(45, valid.front()));
 field.setMaxLength(12);
 EXPECT_EQ(field.getText().size(), 12U);
 field.setMaxLength(40);
 field.setText("");
 const std::string input = std::string("a\n\t") + valid.front();
 field.writeText(input);
 EXPECT_EQ(field.getText(), std::string("a") + valid.front());
 field.enabled = false;
 field.writeText("ignored");
 EXPECT_EQ(field.getText(), std::string("a") + valid.front());
}

void addLocalCycleLabels(client::gui::screen::Screen& screen, client::Minecraft& minecraft,
                         client::option::GameOptions& options) {
 std::string title = "Mode";
 std::string first = "First";
 std::string second = "Second";
 std::string third = "Third";
 client::gui::screen::option::OptionGuiBuilder builder(screen, minecraft, options);
 builder.intCycle(0, 0, "difficulty", title.c_str(), {first.c_str(), second.c_str(), third.c_str()},
                  &client::option::GameOptions::difficulty);
 std::string translatedTitle = "Translated";
 std::string enabledKey = "options.on";
 std::string disabledKey = "options.off";
 builder.i18nCycle(0, 24, "invertYMouse", translatedTitle.c_str(),
                   {enabledKey.c_str(), disabledKey.c_str()}, &client::option::GameOptions::viewDistance);
 builder.i18nCycle(0, 48, "sound", "Empty", {}, &client::option::GameOptions::viewDistance);
}

TEST(GuiRegression, OptionBuilderOwnsCycleLabelsAndNormalizesValues) {
 TestMinecraft minecraft(854, 480, false);
 minecraft.options.optionsFile.clear();
 client::gui::screen::Screen screen;
 screen.init(&minecraft, 854, 480);
 minecraft.options.difficulty = -1;
 minecraft.options.viewDistance = 1;
 addLocalCycleLabels(screen, minecraft, minecraft.options);

 auto* intCycle = dynamic_cast<client::gui::widget::OptionButtonWidget*>(screen.buttons()[0].get());
 auto* translatedCycle = dynamic_cast<client::gui::widget::OptionButtonWidget*>(screen.buttons()[1].get());
 auto* emptyCycle = dynamic_cast<client::gui::widget::OptionButtonWidget*>(screen.buttons()[2].get());
 ASSERT_NE(intCycle, nullptr);
 ASSERT_NE(translatedCycle, nullptr);
 ASSERT_NE(emptyCycle, nullptr);
 intCycle->refreshText(minecraft.options);
 translatedCycle->refreshText(minecraft.options);
 emptyCycle->refreshText(minecraft.options);
 EXPECT_EQ(intCycle->text, "Mode: Third");
 EXPECT_EQ(translatedCycle->text,
           "Translated: " + client::resource::language::I18n::getTranslation("options.off"));
 EXPECT_EQ(emptyCycle->text, "Empty");
}

TEST(GuiRegression, SliderRefreshClampsAndInactiveReleaseDoesNotMutate) {
 TestMinecraft minecraft(854, 480, false);
 minecraft.options.optionsFile.clear();
 const auto index = client::option::OptionRegistry::indexOf("music");
 ASSERT_TRUE(index.has_value());
 client::gui::widget::SliderWidget disabled(0, 0, 0, *index, minecraft, "Music", 0.5f, nullptr);
 minecraft.options.musicVolume = 2.0f;
 disabled.refreshText(minecraft.options);
 EXPECT_FLOAT_EQ(disabled.value, 1.0f);
 minecraft.options.musicVolume = -0.5f;
 disabled.refreshText(minecraft.options);
 EXPECT_FLOAT_EQ(disabled.value, 0.0f);

 minecraft.options.musicVolume = 0.42f;
 disabled.value = 0.9f;
 disabled.active = false;
 disabled.dragging = true;
 disabled.mouseReleased(120, 0);
 EXPECT_FLOAT_EQ(minecraft.options.musicVolume, 0.42f);
 EXPECT_FALSE(disabled.dragging);

 client::gui::widget::SliderWidget hidden(0, 0, 0, *index, minecraft, "Music", 0.9f, nullptr);
 hidden.visible = false;
 hidden.dragging = true;
 hidden.mouseReleased(120, 0);
 EXPECT_FLOAT_EQ(minecraft.options.musicVolume, 0.42f);
 EXPECT_FALSE(hidden.dragging);
}

TEST(GuiRegression, NavigationDiscardsQueuedScreenInput) {
 ActiveWindowGuard activeWindow;
 client::input::InputSystem input;
 input.clearOnDeactivate();
 TestMinecraft minecraft(854, 480, false);
 minecraft.options.optionsFile.clear();
 auto first = std::make_unique<NavigationScreen>();
 NavigationScreen* firstPtr = first.get();
 minecraft.setScreen(std::move(first));
 input.pushMouseButtonEvent(0, true, 10, 10);
 input.pushMouseButtonEvent(1, true, 20, 20);
 input.pushKeyEvent(client::input::keys::kEscape, true);
 input.drainScreenEvents(*firstPtr);

 EXPECT_EQ(firstPtr->mouseEvents, 1);
 EXPECT_EQ(firstPtr->keyboardEvents, 0);
 ASSERT_NE(minecraft.currentScreen(), nullptr);
 auto* newScreen = dynamic_cast<CountingScreen*>(minecraft.currentScreen());
 ASSERT_NE(newScreen, nullptr);
 input.drainScreenEvents(*newScreen);
 EXPECT_EQ(newScreen->mouseEvents, 0);
 EXPECT_EQ(newScreen->keyboardEvents, 0);
 input.clearOnDeactivate();
}

TEST(GuiRegression, EscapeCancelsKeybindCaptureAndReturnsFromVideoOptions) {
 TestMinecraft minecraft(854, 480, false);
 minecraft.options.optionsFile.clear();
 const std::string originalKey = minecraft.options.getKeybindKey(0);
 auto keybinds = std::make_unique<client::gui::screen::option::KeybindsScreen>(
     [] { return std::make_unique<EmptyScreen>(); }, &minecraft.options);
 client::gui::screen::option::KeybindsScreen* keybindsPtr = keybinds.get();
 minecraft.setScreen(std::move(keybinds));
 ASSERT_FALSE(keybindsPtr->buttons().empty());
 const std::string originalButtonText = keybindsPtr->buttons()[0]->text;
 auto* firstButton = dynamic_cast<client::gui::widget::ActionButtonWidget*>(keybindsPtr->buttons()[0].get());
 ASSERT_NE(firstButton, nullptr);
 ASSERT_TRUE(static_cast<bool>(firstButton->onClick));
 firstButton->onClick();
 keybindsPtr->keyPressed(0, client::input::keys::kEscape);
 EXPECT_EQ(minecraft.options.getKeybindKey(0), originalKey);
 EXPECT_EQ(keybindsPtr->buttons()[0]->text, originalButtonText);

 int parentCalls = 0;
 auto video = std::make_unique<client::gui::screen::option::VideoOptionsScreen>(
     [&parentCalls] {
      ++parentCalls;
      return std::make_unique<EmptyScreen>();
     },
     &minecraft.options);
 minecraft.setScreen(std::move(video));
 ASSERT_NE(minecraft.currentScreen(), nullptr);
 minecraft.currentScreen()->keyPressed(0, client::input::keys::kEscape);
 EXPECT_EQ(parentCalls, 1);
 EXPECT_NE(dynamic_cast<EmptyScreen*>(minecraft.currentScreen()), nullptr);
}
}
}
