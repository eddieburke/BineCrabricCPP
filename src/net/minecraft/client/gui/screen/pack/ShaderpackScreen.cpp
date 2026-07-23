#include "net/minecraft/client/gui/screen/pack/ShaderpackScreen.hpp"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <functional>
#include <iomanip>
#include <sstream>
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/gui/layout/OptionsLayout.hpp"
#include "net/minecraft/client/gui/layout/ScreenLayout.hpp"
#include "net/minecraft/client/gui/widget/ButtonWidget.hpp"
#include "net/minecraft/client/input/KeyCodes.hpp"
#include "net/minecraft/client/render/GameRenderer.hpp"
#include "net/minecraft/client/render/RenderSystem.hpp"
#include "net/minecraft/client/render/shaderpack/ShaderPackManager.hpp"

namespace net::minecraft::client::gui::screen::pack {
namespace shaderpack = net::minecraft::client::render::shaderpack;
namespace {
using shaderpack::PackSetting;
using shaderpack::SettingType;
using net::minecraft::client::render::RenderSystem;

constexpr int kListTop = 42;
constexpr int kSectionLabelHeight = 14;
constexpr int kSectionGap = 8;
constexpr int kScrollStep = 24;
constexpr int kButtonWidth = 150;
constexpr int kButtonHeight = 20;

class ShaderSliderWidget : public widget::ButtonWidget {
 public:
 using Change = std::function<float(float)>;
 using Format = std::function<std::string()>;
 ShaderSliderWidget(int x, int y, int width, int height, std::string text, float value, Change change, Format format)
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

float sliderPosition(const PackSetting& setting, const std::string& rawValue) {
 const double range = setting.maximum - setting.minimum;
 if(range <= 0.0) {
  return 0.0f;
 }
 const double current = std::strtod(rawValue.c_str(), nullptr);
 return static_cast<float>(std::clamp((current - setting.minimum) / range, 0.0, 1.0));
}

float applySliderPosition(shaderpack::ShaderPackManager* manager, const PackSetting& setting, float position) {
 const double range = setting.maximum - setting.minimum;
 if(range <= 0.0 || manager == nullptr) {
  return 0.0f;
 }
 double val = setting.minimum + static_cast<double>(std::clamp(position, 0.0f, 1.0f)) * range;
 if(setting.step > 0.0) {
  val = setting.minimum + std::round((val - setting.minimum) / setting.step) * setting.step;
 }
 val = std::clamp(val, setting.minimum, setting.maximum);
 std::string next = (setting.type == SettingType::Int) ? std::to_string(static_cast<int>(val)) : std::to_string(val);
 manager->setSetting(setting.key, next);
 return sliderPosition(setting, next);
}

std::string settingLabel(const PackSetting& setting, const std::string& value) {
 std::ostringstream text;
 text << setting.label << ": ";
 if(setting.type == SettingType::Bool) {
  text << (value == "0" ? "OFF" : "ON");
 } else if(setting.type == SettingType::Int) {
  text << static_cast<int>(std::strtod(value.c_str(), nullptr));
 } else {
  text << std::fixed << std::setprecision(2) << std::strtod(value.c_str(), nullptr);
 }
 return text.str();
}
} // namespace

ShaderpackScreen::ShaderpackScreen(ParentFactory parentFactory) : parentFactory_(std::move(parentFactory)) {
}

void ShaderpackScreen::init() {
 scrollOffset_ = 0;
 rebuildLayout();
}

void ShaderpackScreen::rebuildLayout() {
 buttons_.clear();
 settingWidgets_.clear();
 sectionHeaders_.clear();
 title_ = "Shaderpack Settings";

 if(minecraft() == nullptr || minecraft()->gameRenderer == nullptr ||
    minecraft()->gameRenderer->shaderPacks() == nullptr) {
  return;
 }
 auto* manager = minecraft()->gameRenderer->shaderPacks();
 const auto packs = manager->available();
 int contentY = 0;

 sectionHeaders_.push_back({"Installed Shaderpacks", contentY});
 contentY += kSectionLabelHeight;

 addActionButton(layout::optionsGridX(width(), 0),
                 listTop_ + contentY,
                 kButtonWidth,
                 kButtonHeight,
                 std::string("None") + (manager->active() ? "" : " (selected)"),
                 [this] {
                  if(minecraft() != nullptr && minecraft()->gameRenderer != nullptr &&
                     minecraft()->gameRenderer->shaderPacks() != nullptr) {
                   minecraft()->gameRenderer->shaderPacks()->select("");
                   rebuildLayout();
                  }
                 });

 settingWidgets_.push_back({"", static_cast<int>(buttons_.size() - 1), contentY});

 for(std::size_t i = 0; i < packs.size(); ++i) {
  const auto pack = packs[i];
  const int row = static_cast<int>((i + 1) / 2);
  const int col = static_cast<int>((i + 1) % 2);
  const int widgetY = contentY + row * layout::kRowSpacing;
  addActionButton(layout::optionsGridX(width(), col),
                  listTop_ + widgetY,
                  kButtonWidth,
                  kButtonHeight,
                  pack.name + (pack.valid ? "" : " (invalid)"),
                  [this, key = pack.key] {
                   if(minecraft() != nullptr && minecraft()->gameRenderer != nullptr &&
                      minecraft()->gameRenderer->shaderPacks() != nullptr) {
                    minecraft()->gameRenderer->shaderPacks()->select(key);
                    rebuildLayout();
                   }
                  });
  settingWidgets_.push_back({pack.key, static_cast<int>(buttons_.size() - 1), widgetY});
 }

 contentY += static_cast<int>((packs.size() + 2) / 2) * layout::kRowSpacing + kSectionGap;

 const shaderpack::PackManifest* manifest = manager->activeManifest();
 if(manifest != nullptr && !manifest->settings.empty()) {
  sectionHeaders_.push_back({"Shader Options", contentY});
  contentY += kSectionLabelHeight;

  for(std::size_t i = 0; i < manifest->settings.size(); ++i) {
   const PackSetting setting = manifest->settings[i];
   const int row = static_cast<int>(i / 2);
   const int col = static_cast<int>(i % 2);
   const int widgetY = contentY + row * layout::kRowSpacing;
   const int widgetIndex = static_cast<int>(buttons_.size());
   const int x = layout::optionsGridX(width(), col);

   if(setting.type == SettingType::Bool) {
    addActionButton(x,
                    listTop_ + widgetY,
                    kButtonWidth,
                    kButtonHeight,
                    settingLabel(setting, manager->settingValue(setting.key)),
                    [this, setting, widgetIndex] {
                     if(minecraft() == nullptr || minecraft()->gameRenderer == nullptr ||
                        minecraft()->gameRenderer->shaderPacks() == nullptr) {
                      return;
                     }
                     auto* current = minecraft()->gameRenderer->shaderPacks();
                     const std::string oldValue = current->settingValue(setting.key);
                     const std::string next = (oldValue == "0") ? "1" : "0";
                     current->setSetting(setting.key, next);
                     if(widgetIndex >= 0 && widgetIndex < static_cast<int>(buttons_.size()) &&
                        buttons_[static_cast<std::size_t>(widgetIndex)] != nullptr) {
                      buttons_[static_cast<std::size_t>(widgetIndex)]->text = settingLabel(setting, next);
                     }
                    });
   } else {
    const std::string val = manager->settingValue(setting.key);
    addButton<ShaderSliderWidget>(
        x,
        listTop_ + widgetY,
        kButtonWidth,
        kButtonHeight,
        settingLabel(setting, val),
        sliderPosition(setting, val),
        [this, setting](float pos) {
         if(minecraft() == nullptr || minecraft()->gameRenderer == nullptr) {
          return pos;
         }
         return applySliderPosition(minecraft()->gameRenderer->shaderPacks(), setting, pos);
        },
        [this, setting] {
         if(minecraft() == nullptr || minecraft()->gameRenderer == nullptr ||
            minecraft()->gameRenderer->shaderPacks() == nullptr) {
          return std::string();
         }
         return settingLabel(setting, minecraft()->gameRenderer->shaderPacks()->settingValue(setting.key));
        });
   }
   settingWidgets_.push_back({setting.key, widgetIndex, widgetY});
  }
  contentY += static_cast<int>((manifest->settings.size() + 1) / 2) * layout::kRowSpacing + kSectionGap;
 }

 contentHeight_ = std::max(0, contentY - kSectionGap);
 const int doneY = height() - 28;
 listTop_ = kListTop;
 listBottom_ = std::max(listTop_, doneY - kSectionGap);

 addActionButton(layout::centerBtnX(width()),
                 doneY,
                 kButtonWidth,
                 kButtonHeight,
                 "Done",
                 [this] { navigateTo(parentFactory_); });

 maxScroll_ = std::max(0, contentHeight_ - (listBottom_ - listTop_));
 scrollOffset_ = std::clamp(scrollOffset_, 0, maxScroll_);
 updateListLayout();
}

void ShaderpackScreen::updateListLayout() {
 for(const auto& widgetEntry : settingWidgets_) {
  if(widgetEntry.widgetIndex < 0 || widgetEntry.widgetIndex >= static_cast<int>(buttons_.size())) {
   continue;
  }
  auto& button = buttons_[static_cast<std::size_t>(widgetEntry.widgetIndex)];
  if(button == nullptr) {
   continue;
  }
  button->y = listTop_ + widgetEntry.contentY - scrollOffset_;
  button->visible = button->y >= listTop_ && button->y + button->height <= listBottom_;
 }
}

void ShaderpackScreen::scrollBy(int amount) {
 if(maxScroll_ <= 0) {
  return;
 }
 scrollOffset_ = std::clamp(scrollOffset_ + amount, 0, maxScroll_);
 updateListLayout();
}

void ShaderpackScreen::mouseScrolled(int mouseX, int mouseY, int delta) {
 (void)mouseX;
 if(mouseY < listTop_ || mouseY > listBottom_ || delta == 0) {
  return;
 }
 scrollBy(delta < 0 ? kScrollStep : -kScrollStep);
}

void ShaderpackScreen::keyPressed(char character, int keyCode) {
 if(arrowUpPressed(keyCode)) {
  scrollBy(-kScrollStep);
  return;
 }
 if(arrowDownPressed(keyCode)) {
  scrollBy(kScrollStep);
  return;
 }
 if(escapePressed(keyCode)) {
  navigateTo(parentFactory_);
  return;
 }
 Screen::keyPressed(character, keyCode);
}

void ShaderpackScreen::render(int mouseX, int mouseY, float tickDelta) {
 renderBackground();
 fill(0, listBottom_, width(), height(), 0xE0101010U);
 if(textRenderer() != nullptr) {
  drawCenteredTextWithShadow(*textRenderer(), title_, width() / 2, 12, 0xFFFFFF);
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
} // namespace net::minecraft::client::gui::screen::pack

