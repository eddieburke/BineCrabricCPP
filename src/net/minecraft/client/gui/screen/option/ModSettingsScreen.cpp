#include "net/minecraft/client/gui/screen/option/ModSettingsScreen.hpp"
#include <algorithm>
#include <cmath>
#include <functional>
#include <iomanip>
#include <sstream>
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/gui/layout/ScreenLayout.hpp"
#include "net/minecraft/client/gui/widget/ActionButtonWidget.hpp"
#include "net/minecraft/client/gui/widget/ButtonWidget.hpp"
#include "net/minecraft/client/input/KeyCodes.hpp"
#include "net/minecraft/client/render/RenderSystem.hpp"
#include "net/minecraft/client/resource/language/I18n.hpp"
#include "net/minecraft/mod/ModSettingsRegistry.hpp"
#include "net/minecraft/mod/ScreenUi.hpp"
namespace net::minecraft::client::gui::screen::option {
namespace {
using net::minecraft::client::render::RenderSystem;
constexpr int kListTop = 42;
constexpr int kSectionLabelHeight = 14;
constexpr int kSectionGap = 8;
constexpr int kScrollStep = 24;
constexpr int kButtonWidth = 150;
constexpr int kButtonHeight = 20;
constexpr int kDoneYInset = 28;
class ModSliderWidget : public widget::ButtonWidget {
 public:
 using Change = std::function<float(float)>;
 using Format = std::function<std::string()>;
 ModSliderWidget(int x, int y, int width, int height, std::string text, float value, Change change, Format format)
     : ButtonWidget(-1, x, y, width, height, std::move(text)),
       value_(std::clamp(value, 0.0f, 1.0f)),
       change_(std::move(change)),
       format_(std::move(format)) {
 }
 [[nodiscard]] int getYImage(bool) const override {
  return 0;
 }
 void renderBackground(int mouseX, int) override {
  if(!visible) {
   return;
  }
  if(dragging_) {
   updateValue(mouseX);
  }
  RenderSystem::color4f(1.0f, 1.0f, 1.0f, 1.0f);
  const int knobX = x + static_cast<int>(value_ * static_cast<float>(width - 8));
  drawTexture(knobX, y, 0, 66, 4, 20);
  drawTexture(knobX + 4, y, 196, 66, 4, 20);
 }
 [[nodiscard]] bool isMouseOver(int mouseX, int mouseY) const override {
  return ButtonWidget::isMouseOver(mouseX, mouseY);
 }
 void onMouseDown(int mouseX, int) override {
  dragging_ = true;
  updateValue(mouseX);
 }
 void mouseReleased(int, int) override {
  dragging_ = false;
 }

 private:
 void updateValue(int mouseX) {
  const float position =
      std::clamp(static_cast<float>(mouseX - (x + 4)) / static_cast<float>(std::max(1, width - 8)), 0.0f, 1.0f);
  value_ = change_ ? std::clamp(change_(position), 0.0f, 1.0f) : position;
  if(format_) {
   text = format_();
  }
 }
 float value_ = 0.0f;
 bool dragging_ = false;
 Change change_;
 Format format_;
};
float sliderPosition(const net::minecraft::mod::ModSettingDef& setting) {
 const float range = setting.sliderMax - setting.sliderMin;
 if(range <= 0.0f) {
  return 0.0f;
 }
 return std::clamp((setting.floatCurrent - setting.sliderMin) / range, 0.0f, 1.0f);
}
float applySliderPosition(net::minecraft::mod::ModSettingDef& setting, float position) {
 const float range = setting.sliderMax - setting.sliderMin;
 if(range <= 0.0f) {
  setting.floatCurrent = setting.sliderMin;
  return 0.0f;
 }
 float value = setting.sliderMin + std::clamp(position, 0.0f, 1.0f) * range;
 const float step = setting.sliderStep > 0.0f ? setting.sliderStep : (setting.sliderInteger ? 1.0f : 0.0f);
 if(step > 0.0f) {
  value = setting.sliderMin + std::round((value - setting.sliderMin) / step) * step;
 }
 if(setting.sliderInteger) {
  value = std::round(value);
 }
 setting.floatCurrent = std::clamp(value, setting.sliderMin, setting.sliderMax);
 return sliderPosition(setting);
}
std::string formatSliderLabel(const net::minecraft::mod::ModSettingDef& setting) {
 std::ostringstream label;
 label << setting.label << ": ";
 if(setting.sliderInteger) {
  label << static_cast<int>(setting.floatCurrent);
 } else {
  label << std::fixed << std::setprecision(setting.sliderDecimals) << setting.floatCurrent;
 }
 return label.str();
}
std::string formatToggleLabel(const net::minecraft::mod::ModSettingDef& setting) {
 std::string onLabel = net::minecraft::client::resource::language::I18n::getTranslation("options.on");
 std::string offLabel = net::minecraft::client::resource::language::I18n::getTranslation("options.off");
 if(onLabel.empty()) {
  onLabel = "ON";
 }
 if(offLabel.empty()) {
  offLabel = "OFF";
 }
 return setting.label + ": " + (setting.boolCurrent ? onLabel : offLabel);
}
std::string formatOptionsLabel(const net::minecraft::mod::ModSettingDef& setting) {
 if(setting.options.empty() || setting.optionCurrent < 0 ||
    setting.optionCurrent >= static_cast<int>(setting.options.size())) {
  return setting.label + ": " + std::to_string(setting.optionCurrent);
 }
 return setting.label + ": " + setting.options[static_cast<std::size_t>(setting.optionCurrent)];
}
std::string formatKeybindLabel(const net::minecraft::mod::ModKeybindDef& keybind, bool selected) {
 const std::string keyName = client::input::keyDisplayName(keybind.currentKeyCode);
 return keybind.label + ": " + (selected ? "> " + keyName + " <" : keyName);
}
} // namespace
ModSettingsScreen::ModSettingsScreen(ParentFactory parentFactory) : parentFactory_(std::move(parentFactory)) {
}
ModSettingsScreen::ParentFactory ModSettingsScreen::modPagesFactory() const {
 const ParentFactory parent = parentFactory_;
 return [parent] { return std::make_unique<ModSettingsScreen>(parent); };
}
void ModSettingsScreen::init() {
 selectedKeybindIndex_ = -1;
 scrollOffset_ = 0;
 rebuildLayout();
}
void ModSettingsScreen::rebuildLayout() {
 title_ = "Mod Settings";
 buttons_.clear();
 settingWidgets_.clear();
 keybindWidgets_.clear();
 sectionHeaders_.clear();
 injectedPages_.clear();
 auto& registry = net::minecraft::mod::ModSettingsRegistry::instance();
 const auto allSettings = registry.getAllSettings();
 const auto allKeybinds = registry.getAllKeybinds();
 const auto& modNames = registry.getModNames();
 int contentY = 0;
 for(const auto& [modId, settings] : allSettings) {
  const auto nameIt = modNames.find(modId);
  sectionHeaders_.push_back({nameIt == modNames.end() ? modId : nameIt->second, contentY});
  contentY += kSectionLabelHeight;
  for(std::size_t i = 0; i < settings.size(); ++i) {
   auto* setting = settings[i];
   const int row = static_cast<int>(i / 2);
   const int column = static_cast<int>(i % 2);
   const int widgetY = contentY + row * layout::kRowSpacing;
   const int widgetIndex = static_cast<int>(buttons_.size());
   const int x = layout::optionsGridX(width(), column);
   if(setting->kind == net::minecraft::mod::ModSettingDef::Slider) {
    const std::string key = setting->key;
    addButton<ModSliderWidget>(
        x,
        listTop_ + widgetY,
        kButtonWidth,
        kButtonHeight,
        formatSliderLabel(*setting),
        sliderPosition(*setting),
        [modId, key](float position) {
         auto* current = net::minecraft::mod::ModSettingsRegistry::instance().findSetting(modId, key);
         return current == nullptr ? position : applySliderPosition(*current, position);
        },
        [modId, key] {
         const auto* current =
             net::minecraft::mod::ModSettingsRegistry::instance().findSetting(modId, key);
         return current == nullptr ? std::string() : formatSliderLabel(*current);
        });
   } else if(setting->kind == net::minecraft::mod::ModSettingDef::Options) {
    addActionButton(x,
                    listTop_ + widgetY,
                    kButtonWidth,
                    kButtonHeight,
                    formatOptionsLabel(*setting),
                    [this, modId, key = setting->key, widgetIndex] {
                     auto& settingsRegistry = net::minecraft::mod::ModSettingsRegistry::instance();
                     auto* current = settingsRegistry.findSetting(modId, key);
                     if(current == nullptr || current->options.empty()) {
                      return;
                     }
                     current->optionCurrent = (current->optionCurrent + 1) % static_cast<int>(current->options.size());
                     if(widgetIndex >= 0 && widgetIndex < static_cast<int>(buttons_.size()) &&
                        buttons_[static_cast<std::size_t>(widgetIndex)] != nullptr) {
                      buttons_[static_cast<std::size_t>(widgetIndex)]->text =
                          formatOptionsLabel(*current);
                     }
                     settingsRegistry.save();
                    });
   } else {
    addActionButton(x,
                    listTop_ + widgetY,
                    kButtonWidth,
                    kButtonHeight,
                    formatToggleLabel(*setting),
                    [this, modId, key = setting->key, widgetIndex] {
                     auto& settingsRegistry = net::minecraft::mod::ModSettingsRegistry::instance();
                     auto* current = settingsRegistry.findSetting(modId, key);
                     if(current == nullptr) {
                      return;
                     }
                     current->boolCurrent = !current->boolCurrent;
                     if(widgetIndex >= 0 && widgetIndex < static_cast<int>(buttons_.size()) &&
                        buttons_[static_cast<std::size_t>(widgetIndex)] != nullptr) {
                      buttons_[static_cast<std::size_t>(widgetIndex)]->text =
                          formatToggleLabel(*current);
                     }
                     settingsRegistry.save();
                    });
   }
   settingWidgets_.push_back({modId, setting->key, widgetIndex, widgetY});
  }
  contentY += static_cast<int>((settings.size() + 1) / 2) * layout::kRowSpacing + kSectionGap;
 }
 if(!allKeybinds.empty()) {
  sectionHeaders_.push_back({"Controls", contentY});
  contentY += kSectionLabelHeight;
  for(std::size_t i = 0; i < allKeybinds.size(); ++i) {
   auto* keybind = allKeybinds[i];
   const int row = static_cast<int>(i / 2);
   const int column = static_cast<int>(i % 2);
   const int widgetY = contentY + row * layout::kRowSpacing;
   const int widgetIndex = static_cast<int>(buttons_.size());
   const int keybindIndex = static_cast<int>(keybindWidgets_.size());
   addActionButton(layout::optionsGridX(width(), column),
                   listTop_ + widgetY,
                   kButtonWidth,
                   kButtonHeight,
                   formatKeybindLabel(*keybind, selectedKeybindIndex_ == keybindIndex),
                   [this, keybindIndex] {
                    auto keybinds = net::minecraft::mod::ModSettingsRegistry::instance().getAllKeybinds();
                    if(keybindIndex < 0 || keybindIndex >= static_cast<int>(keybinds.size())) {
                     return;
                    }
                    selectedKeybindIndex_ = keybindIndex;
                    for(std::size_t j = 0; j < keybindWidgets_.size() && j < keybinds.size(); ++j) {
                     const int index = keybindWidgets_[j].widgetIndex;
                     if(index >= 0 && index < static_cast<int>(buttons_.size()) &&
                        buttons_[static_cast<std::size_t>(index)] != nullptr) {
                      buttons_[static_cast<std::size_t>(index)]->text =
                          formatKeybindLabel(*keybinds[j], static_cast<int>(j) == keybindIndex);
                     }
                    }
                   });
   keybindWidgets_.push_back({keybind->id, widgetIndex, widgetY});
  }
  contentY += static_cast<int>((allKeybinds.size() + 1) / 2) * layout::kRowSpacing + kSectionGap;
 }
 contentHeight_ = std::max(0, contentY - kSectionGap);
 const std::size_t firstInjectedWidget = buttons_.size();
 int injectedY = 0;
 publishScreenUi(net::minecraft::mod::screen_regions::kFooter, &injectedY);
 for(std::size_t i = firstInjectedWidget; i < buttons_.size(); ++i) {
  auto* action = dynamic_cast<widget::ActionButtonWidget*>(buttons_[i].get());
  if(action != nullptr) {
   injectedPages_.push_back({action->text, action->onClick});
  }
 }
 buttons_.resize(firstInjectedWidget);
 const std::size_t injectedWidgetCount = injectedPages_.size();
 const int doneY = height() - kDoneYInset;
 const int injectedRows = static_cast<int>((injectedWidgetCount + 1) / 2);
 const int injectedTop = doneY - injectedRows * layout::kRowSpacing;
 listTop_ = kListTop;
 listBottom_ = std::max(listTop_, injectedTop - kSectionGap);
 for(std::size_t i = 0; i < injectedWidgetCount; ++i) {
  const int row = static_cast<int>(i / 2);
  const int column = static_cast<int>(i % 2);
  const int buttonX =
      injectedWidgetCount == 1 ? layout::centerBtnX(width()) : layout::optionsGridX(width(), column);
  addActionButton(buttonX,
                  injectedTop + row * layout::kRowSpacing,
                  kButtonWidth,
                  kButtonHeight,
                  injectedPages_[i].text,
                  injectedPages_[i].onClick);
 }
 addActionButton(layout::centerBtnX(width()),
                 doneY,
                 kButtonWidth,
                 kButtonHeight,
                 net::minecraft::client::resource::language::I18n::getTranslation("gui.done"),
                 [this] {
                  net::minecraft::mod::ModSettingsRegistry::instance().save();
                  selectedKeybindIndex_ = -1;
                  if(parentFactory_) {
                   navigateTo(parentFactory_);
                  } else {
                   closeScreen();
                  }
                 });
 maxScroll_ = std::max(0, contentHeight_ - (listBottom_ - listTop_));
 scrollOffset_ = std::clamp(scrollOffset_, 0, maxScroll_);
 updateListLayout();
}
void ModSettingsScreen::updateListLayout() {
 for(const auto& settingWidget : settingWidgets_) {
  if(settingWidget.widgetIndex < 0 || settingWidget.widgetIndex >= static_cast<int>(buttons_.size())) {
   continue;
  }
  auto& button = buttons_[static_cast<std::size_t>(settingWidget.widgetIndex)];
  if(button == nullptr) {
   continue;
  }
  button->y = listTop_ + settingWidget.contentY - scrollOffset_;
  button->visible = button->y >= listTop_ && button->y + button->height <= listBottom_;
 }
 for(const auto& keybindWidget : keybindWidgets_) {
  if(keybindWidget.widgetIndex < 0 || keybindWidget.widgetIndex >= static_cast<int>(buttons_.size())) {
   continue;
  }
  auto& button = buttons_[static_cast<std::size_t>(keybindWidget.widgetIndex)];
  if(button == nullptr) {
   continue;
  }
  button->y = listTop_ + keybindWidget.contentY - scrollOffset_;
  button->visible = button->y >= listTop_ && button->y + button->height <= listBottom_;
 }
}
void ModSettingsScreen::scrollBy(int amount) {
 if(maxScroll_ <= 0) {
  return;
 }
 scrollOffset_ = std::clamp(scrollOffset_ + amount, 0, maxScroll_);
 updateListLayout();
}
void ModSettingsScreen::mouseScrolled(int mouseX, int mouseY, int delta) {
 (void)mouseX;
 if(mouseY < listTop_ || mouseY > listBottom_ || delta == 0) {
  return;
 }
 scrollBy(delta < 0 ? kScrollStep : -kScrollStep);
}
void ModSettingsScreen::render(int mouseX, int mouseY, float tickDelta) {
 renderBackground();
 fill(0, listBottom_, width(), height(), 0xE0101010U);
 if(textRenderer() != nullptr) {
  drawCenteredTextWithShadow(*textRenderer(), title_, width() / 2, 12, 0xFFFFFF);
  drawCenteredTextWithShadow(*textRenderer(), "Click to change", width() / 2, 25, 0xA0A0A0);
  for(const auto& header : sectionHeaders_) {
   const int y = listTop_ + header.contentY - scrollOffset_;
   if(y >= listTop_ && y + 8 <= listBottom_) {
    drawTextWithShadow(*textRenderer(), header.text, layout::twoColumnLeftX(width()), y, 0xFFFFA0);
   }
  }
  if(maxScroll_ > 0) {
   const int trackX = width() / 2 + 164;
   const int trackHeight = listBottom_ - listTop_;
   const int thumbHeight = std::max(16, trackHeight * trackHeight / std::max(trackHeight, contentHeight_));
   const int thumbTravel = std::max(0, trackHeight - thumbHeight);
   const int thumbY = listTop_ + (maxScroll_ == 0 ? 0 : scrollOffset_ * thumbTravel / maxScroll_);
   fill(trackX, listTop_, trackX + 2, listBottom_, 0x60000000);
   fill(trackX, thumbY, trackX + 2, thumbY + thumbHeight, 0xFFC0C0C0);
  }
 }
 Screen::render(mouseX, mouseY, tickDelta);
}
void ModSettingsScreen::mouseReleased(int mouseX, int mouseY, int button) {
 Screen::mouseReleased(mouseX, mouseY, button);
}
void ModSettingsScreen::keyPressed(char character, int keyCode) {
 if(selectedKeybindIndex_ < 0) {
  if(arrowUpPressed(keyCode)) {
   scrollBy(-kScrollStep);
   return;
  }
  if(arrowDownPressed(keyCode)) {
   scrollBy(kScrollStep);
   return;
  }
  if(escapePressed(keyCode)) {
   net::minecraft::mod::ModSettingsRegistry::instance().save();
   if(parentFactory_) {
    navigateTo(parentFactory_);
   } else {
    closeScreen();
   }
   return;
  }
  Screen::keyPressed(character, keyCode);
  return;
 }
 auto& registry = net::minecraft::mod::ModSettingsRegistry::instance();
 auto keybinds = registry.getAllKeybinds();
 if(selectedKeybindIndex_ < static_cast<int>(keybinds.size())) {
  keybinds[static_cast<std::size_t>(selectedKeybindIndex_)]->currentKeyCode = keyCode;
  registry.save();
  if(selectedKeybindIndex_ < static_cast<int>(keybindWidgets_.size())) {
   const int index = keybindWidgets_[static_cast<std::size_t>(selectedKeybindIndex_)].widgetIndex;
   if(index >= 0 && index < static_cast<int>(buttons_.size()) &&
      buttons_[static_cast<std::size_t>(index)] != nullptr) {
    buttons_[static_cast<std::size_t>(index)]->text =
        formatKeybindLabel(*keybinds[static_cast<std::size_t>(selectedKeybindIndex_)], false);
   }
  }
 }
 selectedKeybindIndex_ = -1;
}
} // namespace net::minecraft::client::gui::screen::option
