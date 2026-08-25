#include "net/minecraft/client/gui/screen/option/KeybindsScreen.hpp"
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/gui/layout/ScreenLayout.hpp"
#include "net/minecraft/client/input/Keys.hpp"
#include "net/minecraft/client/resource/language/I18n.hpp"
#include "net/minecraft/mod/ModSettingsRegistry.hpp"
namespace net::minecraft::client::gui::screen::option {
namespace {
constexpr int kButtonWidth = 150;
constexpr int kButtonHeight = 20;
constexpr int kDoneYInset = 28;
}
KeybindsScreen::KeybindsScreen(ParentFactory parentFactory, client_option::GameOptions* gameOptions)
    : parentFactory_(std::move(parentFactory)), gameOptions_(gameOptions) {
}
void KeybindsScreen::init() {
 title_ = resource::language::I18n::getTranslation("controls.title");
 buttons_.clear();
 bindings_.clear();
 scroll_.clear();
 selectedKeyBinding_ = -1;
 if(gameOptions_ == nullptr) {
  return;
 }
 const int listTop = layout::OptionsListScroll::kModListTop;
 int contentY = 0;
 scroll_.addHeader("Minecraft", contentY);
 contentY += layout::OptionsListScroll::kSectionLabelHeight;
 for(int i = 0; i < client_option::GameOptions::kKeybindCount; ++i) {
  const int bindingIndex = static_cast<int>(bindings_.size());
  const int buttonIndex = static_cast<int>(buttons_.size());
  const int row = i / 2;
  const int column = i % 2;
  bindings_.push_back({BindingEntry::Source::Vanilla, i, {}, buttonIndex});
  addActionButton(layout::optionsGridX(width(), column),
                  listTop + contentY + row * layout::kRowSpacing,
                  kButtonWidth,
                  kButtonHeight,
                  bindingText(bindings_.back(), false),
                  [this, bindingIndex] { selectKeybind(bindingIndex); });
  scroll_.addEntry(buttonIndex, contentY + row * layout::kRowSpacing);
 }
 contentY += ((client_option::GameOptions::kKeybindCount + 1) / 2) * layout::kRowSpacing +
             layout::OptionsListScroll::kSectionGap;
 const auto modKeybinds = net::minecraft::mod::ModSettingsRegistry::instance().getAllKeybinds();
 if(!modKeybinds.empty()) {
  scroll_.addHeader("Mod Controls", contentY);
  contentY += layout::OptionsListScroll::kSectionLabelHeight;
  for(std::size_t i = 0; i < modKeybinds.size(); ++i) {
   const int bindingIndex = static_cast<int>(bindings_.size());
   const int buttonIndex = static_cast<int>(buttons_.size());
   const int row = static_cast<int>(i / 2);
   const int column = static_cast<int>(i % 2);
   bindings_.push_back({BindingEntry::Source::Mod, -1, modKeybinds[i]->id, buttonIndex});
   addActionButton(layout::optionsGridX(width(), column),
                   listTop + contentY + row * layout::kRowSpacing,
                   kButtonWidth,
                   kButtonHeight,
                   bindingText(bindings_.back(), false),
                   [this, bindingIndex] { selectKeybind(bindingIndex); });
   scroll_.addEntry(buttonIndex, contentY + row * layout::kRowSpacing);
  }
  contentY += static_cast<int>((modKeybinds.size() + 1) / 2) * layout::kRowSpacing +
              layout::OptionsListScroll::kSectionGap;
 }
 const int doneY = height() - kDoneYInset;
 scroll_.setViewport(listTop, doneY - layout::OptionsListScroll::kSectionGap);
 scroll_.setContentHeight(std::max(0, contentY - layout::OptionsListScroll::kSectionGap));
 addActionButton(
     layout::centerBtnX(width()), doneY, resource::language::I18n::getTranslation("gui.done"), [this] {
      gameOptions_->save();
      net::minecraft::mod::ModSettingsRegistry::instance().save();
      if(parentFactory_) {
       navigateTo(parentFactory_);
      }
     });
 updateListLayout();
}
void KeybindsScreen::render(int mouseX, int mouseY, float tickDelta) {
 if(scroll_.draggingThumb()) {
  scroll_.mouseDragged(mouseY);
  updateListLayout();
 }
 renderBackground();
 scroll_.renderFooterDim(width(), height());
 if(textRenderer() != nullptr) {
  textRenderer()->drawCenteredWithShadow(title_, width() / 2, 12, 0xFFFFFF);
  textRenderer()->drawCenteredWithShadow("Click to change", width() / 2, 25, 0xA0A0A0);
 }
 scroll_.renderHeaders(textRenderer(), width());
 scroll_.renderScrollbar(width());
 Screen::render(mouseX, mouseY, tickDelta);
}
void KeybindsScreen::selectKeybind(int index) {
 if(index < 0 || index >= static_cast<int>(bindings_.size())) {
  return;
 }
 for(std::size_t i = 0; i < bindings_.size(); ++i) {
  const BindingEntry& binding = bindings_[i];
  if(binding.buttonIndex >= 0 && binding.buttonIndex < static_cast<int>(buttons_.size()) &&
     buttons_[static_cast<std::size_t>(binding.buttonIndex)] != nullptr) {
   buttons_[static_cast<std::size_t>(binding.buttonIndex)]->text =
       bindingText(binding, static_cast<int>(i) == index);
  }
 }
 selectedKeyBinding_ = index;
}
void KeybindsScreen::keyPressed(char character, int keyCode) {
 (void)character;
 if(selectedKeyBinding_ < 0) {
  if(arrowUpPressed(keyCode)) {
   scroll_.scrollBy(-layout::OptionsListScroll::kScrollStep);
   updateListLayout();
   return;
  }
  if(arrowDownPressed(keyCode)) {
   scroll_.scrollBy(layout::OptionsListScroll::kScrollStep);
   updateListLayout();
   return;
  }
  Screen::keyPressed(character, keyCode);
  return;
 }
 if(selectedKeyBinding_ >= static_cast<int>(bindings_.size())) {
  selectedKeyBinding_ = -1;
  return;
 }
 const BindingEntry& binding = bindings_[static_cast<std::size_t>(selectedKeyBinding_)];
 if(binding.source == BindingEntry::Source::Vanilla) {
  gameOptions_->setKeybindKey(binding.vanillaIndex, keyCode);
 } else if(auto* keybind = net::minecraft::mod::ModSettingsRegistry::instance().findKeybind(binding.modId)) {
  keybind->currentKeyCode = keyCode;
  net::minecraft::mod::ModSettingsRegistry::instance().save();
 }
 if(binding.buttonIndex >= 0 && binding.buttonIndex < static_cast<int>(buttons_.size()) &&
    buttons_[static_cast<std::size_t>(binding.buttonIndex)] != nullptr) {
  buttons_[static_cast<std::size_t>(binding.buttonIndex)]->text = bindingText(binding, false);
 }
 selectedKeyBinding_ = -1;
}
void KeybindsScreen::mouseClicked(int mouseX, int mouseY, int button) {
 if(button == 0 && scroll_.mouseClicked(mouseX, mouseY, width())) {
  updateListLayout();
  return;
 }
 Screen::mouseClicked(mouseX, mouseY, button);
}
void KeybindsScreen::mouseReleased(int mouseX, int mouseY, int button) {
 scroll_.mouseReleased();
 Screen::mouseReleased(mouseX, mouseY, button);
}
void KeybindsScreen::mouseScrolled(int mouseX, int mouseY, int delta) {
 (void)mouseX;
 if(scroll_.mouseScrolled(mouseY, delta)) {
  updateListLayout();
 }
}
void KeybindsScreen::updateListLayout() {
 scroll_.apply(buttons_);
}
std::string KeybindsScreen::bindingName(const BindingEntry& binding) const {
 if(binding.source == BindingEntry::Source::Vanilla) {
  return gameOptions_ != nullptr ? gameOptions_->getKeybindName(binding.vanillaIndex) : "?";
 }
 const auto* keybind = net::minecraft::mod::ModSettingsRegistry::instance().findKeybind(binding.modId);
 return keybind != nullptr ? keybind->label : binding.modId;
}
std::string KeybindsScreen::bindingKey(const BindingEntry& binding) const {
 if(binding.source == BindingEntry::Source::Vanilla) {
  return gameOptions_ != nullptr ? gameOptions_->getKeybindKey(binding.vanillaIndex) : "?";
 }
 const auto* keybind = net::minecraft::mod::ModSettingsRegistry::instance().findKeybind(binding.modId);
 return keybind != nullptr ? client::input::keyDisplayName(keybind->currentKeyCode) : "?";
}
std::string KeybindsScreen::bindingText(const BindingEntry& binding, bool selected) const {
 const std::string key = bindingKey(binding);
 return bindingName(binding) + ": " + (selected ? "> " + key + " <" : key);
}
} // namespace net::minecraft::client::gui::screen::option
