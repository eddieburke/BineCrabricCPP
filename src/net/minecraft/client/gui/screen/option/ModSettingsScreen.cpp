#include "net/minecraft/client/gui/screen/option/ModSettingsScreen.hpp"
#include <algorithm>
#include <cmath>
#include <functional>
#include <iomanip>
#include <limits>
#include <sstream>
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/gui/layout/ScreenLayout.hpp"
#include "net/minecraft/client/gui/widget/ActionButtonWidget.hpp"
#include "net/minecraft/client/gui/widget/ButtonWidget.hpp"
#include "net/minecraft/client/input/KeyCodes.hpp"
#include "net/minecraft/client/resource/language/I18n.hpp"
#include "net/minecraft/client/gui/Draw2D.hpp"
#include "net/minecraft/client/render/RenderCore.hpp"
#include "net/minecraft/client/render/RenderType.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/mod/ModSettingsRegistry.hpp"
#include "net/minecraft/mod/ScreenUi.hpp"
namespace net::minecraft::client::gui::screen::option {
namespace {
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
  render::core::setConstColor(1.0f, 1.0f, 1.0f, 1.0f);
  const int knobX = x + static_cast<int>(value_ * static_cast<float>(width - 8));
  {
   const render::RenderPassScope passScope(render::RenderType::guiTextured());
   const float* c = render::core::constColor();
   render::Tessellator& tess = render::INSTANCE;
   tess.startQuads();
   tess.color(c[0], c[1], c[2], c[3]);
   draw::appendAtlasQuad(tess, knobX, y, 0, 66, 4, 20, 0.0f);
   draw::appendAtlasQuad(tess, knobX + 4, y, 196, 66, 4, 20, 0.0f);
   tess.draw();
  }
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
 rebuildLayout();
}
void ModSettingsScreen::rebuildLayout() {
 title_ = "Mod Settings";
 buttons_.clear();
 settingWidgets_.clear();
 keybindWidgets_.clear();
 scroll_.clear();
 auto& registry = net::minecraft::mod::ModSettingsRegistry::instance();
 const auto allSettings = registry.getAllSettings();
 const auto allKeybinds = registry.getAllKeybinds();
 const auto& modNames = registry.getModNames();
 const int listTop = layout::OptionsListScroll::kModListTop;
 int contentY = 0;
 for(const auto& [modId, settings] : allSettings) {
  const auto nameIt = modNames.find(modId);
  scroll_.addHeader(nameIt == modNames.end() ? modId : nameIt->second, contentY);
  contentY += layout::OptionsListScroll::kSectionLabelHeight;
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
        listTop + widgetY,
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
                    listTop + widgetY,
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
                    listTop + widgetY,
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
   scroll_.addEntry(widgetIndex, widgetY);
   settingWidgets_.push_back({modId, setting->key, widgetIndex});
  }
  contentY += static_cast<int>((settings.size() + 1) / 2) * layout::kRowSpacing +
              layout::OptionsListScroll::kSectionGap;
 }
 if(!allKeybinds.empty()) {
  scroll_.addHeader("Controls", contentY);
  contentY += layout::OptionsListScroll::kSectionLabelHeight;
  for(std::size_t i = 0; i < allKeybinds.size(); ++i) {
   auto* keybind = allKeybinds[i];
   const int row = static_cast<int>(i / 2);
   const int column = static_cast<int>(i % 2);
   const int widgetY = contentY + row * layout::kRowSpacing;
   const int widgetIndex = static_cast<int>(buttons_.size());
   const int keybindIndex = static_cast<int>(keybindWidgets_.size());
   addActionButton(layout::optionsGridX(width(), column),
                   listTop + widgetY,
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
   scroll_.addEntry(widgetIndex, widgetY);
   keybindWidgets_.push_back({keybind->id, widgetIndex});
  }
  contentY += static_cast<int>((allKeybinds.size() + 1) / 2) * layout::kRowSpacing +
              layout::OptionsListScroll::kSectionGap;
 }
 const int contentHeight = std::max(0, contentY - layout::OptionsListScroll::kSectionGap);
 const int doneY = height() - kDoneYInset;
 const std::size_t firstInjectedWidget = buttons_.size();
 int injectedY = doneY;
 publishScreenUi(net::minecraft::mod::screen_regions::kFooter, &injectedY);
 int injectedTop = doneY;
 if(firstInjectedWidget < buttons_.size()) {
  int top = std::numeric_limits<int>::max();
  int bottom = std::numeric_limits<int>::min();
  for(std::size_t i = firstInjectedWidget; i < buttons_.size(); ++i) {
   if(buttons_[i] == nullptr) {
    continue;
   }
   top = std::min(top, buttons_[i]->y);
   bottom = std::max(bottom, buttons_[i]->y + buttons_[i]->height);
  }
  if(top <= bottom) {
   const int shift = (doneY - layout::OptionsListScroll::kSectionGap) - bottom;
   for(std::size_t i = firstInjectedWidget; i < buttons_.size(); ++i) {
    if(buttons_[i] != nullptr) {
     buttons_[i]->y += shift;
    }
   }
   injectedTop = top + shift;
  }
 }
 scroll_.setViewport(listTop, std::max(listTop, injectedTop - layout::OptionsListScroll::kSectionGap));
 scroll_.setContentHeight(contentHeight);
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
 updateListLayout();
}
void ModSettingsScreen::updateListLayout() {
 scroll_.apply(buttons_);
}
void ModSettingsScreen::mouseScrolled(int mouseX, int mouseY, int delta) {
 (void)mouseX;
 if(scroll_.mouseScrolled(mouseY, delta)) {
  updateListLayout();
 }
}
void ModSettingsScreen::mouseClicked(int mouseX, int mouseY, int button) {
 if(button == 0 && scroll_.mouseClicked(mouseX, mouseY, width())) {
  updateListLayout();
  return;
 }
 Screen::mouseClicked(mouseX, mouseY, button);
}
void ModSettingsScreen::render(int mouseX, int mouseY, float tickDelta) {
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
void ModSettingsScreen::mouseReleased(int mouseX, int mouseY, int button) {
 scroll_.mouseReleased();
 Screen::mouseReleased(mouseX, mouseY, button);
}
void ModSettingsScreen::keyPressed(char character, int keyCode) {
 if(selectedKeybindIndex_ < 0) {
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
