#include "net/minecraft/client/gui/screen/pack/ShaderpackScreen.hpp"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <functional>
#include <iomanip>
#include <sstream>
#include <utility>
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/gui/Draw2D.hpp"
#include "net/minecraft/client/gui/layout/OptionsLayout.hpp"
#include "net/minecraft/client/gui/layout/ScreenLayout.hpp"
#include "net/minecraft/client/gui/widget/ButtonWidget.hpp"
#include "net/minecraft/client/input/KeyCodes.hpp"
#include "net/minecraft/client/render/GameRenderer.hpp"
#include "net/minecraft/client/render/RenderCore.hpp"
#include "net/minecraft/client/render/RenderType.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/client/render/pipeline/Manager.hpp"
namespace net::minecraft::client::gui::screen::pack {
namespace render = net::minecraft::client::render;
namespace {
using render::PackSetting;
using render::SettingType;
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
float sliderPosition(const PackSetting& setting, const std::string& rawValue) {
 const double range = setting.maximum - setting.minimum;
 if(range <= 0.0) {
  return 0.0f;
 }
 const double current = std::strtod(rawValue.c_str(), nullptr);
 return static_cast<float>(std::clamp((current - setting.minimum) / range, 0.0, 1.0));
}
float applySliderPosition(render::PackManager* manager, const PackSetting& setting, float position) {
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
 std::string display = value;
 if(setting.type == SettingType::Bool) {
  display = value == "0" ? "OFF" : "ON";
 } else if(setting.type == SettingType::Int) {
  display = std::to_string(static_cast<int>(std::strtod(value.c_str(), nullptr)));
 } else if(setting.type == SettingType::Float) {
  std::ostringstream num;
  num << std::fixed << std::setprecision(2) << std::strtod(value.c_str(), nullptr);
  display = num.str();
 }
 const auto labeled = setting.valueLabels.find(value);
 if(labeled != setting.valueLabels.end()) display = labeled->second;
 else {
  const auto labeledDisplay = setting.valueLabels.find(display);
  if(labeledDisplay != setting.valueLabels.end()) display = labeledDisplay->second;
 }
 text << setting.valuePrefix << display << setting.valueSuffix;
 return text.str();
}
} // namespace
ShaderpackScreen::ShaderpackScreen(ParentFactory parentFactory) : parentFactory_(std::move(parentFactory)) {
}
void ShaderpackScreen::init() {
 rebuildLayout();
}
void ShaderpackScreen::rebuildLayout() {
 buttons_.clear();
 scroll_.clear();
 title_ = "Shaderpack Settings";
 if(minecraft() == nullptr || minecraft()->gameRenderer == nullptr ||
    minecraft()->gameRenderer->shaderPacks() == nullptr) {
  return;
 }
 auto* manager = minecraft()->gameRenderer->shaderPacks();
 manager->poll();
 const auto packs = manager->available();
 const bool userPackSelected = std::any_of(packs.begin(), packs.end(), [](const auto& pack) {
  return pack.selected;
 });
 const int listTop = layout::OptionsListScroll::kModListTop;
 int contentY = 0;
 scroll_.addHeader("Installed Shaderpacks", contentY);
 contentY += layout::OptionsListScroll::kSectionLabelHeight;
 addActionButton(layout::optionsGridX(width(), 0),
                 listTop + contentY,
                 kButtonWidth,
                 kButtonHeight,
                 std::string("None") + (userPackSelected ? "" : " (selected)"),
                 [this] {
                  if(minecraft() != nullptr && minecraft()->gameRenderer != nullptr &&
                     minecraft()->gameRenderer->shaderPacks() != nullptr) {
                   minecraft()->gameRenderer->shaderPacks()->select("");
                   rebuildLayout();
                  }
                 });
 scroll_.addEntry(static_cast<int>(buttons_.size() - 1), contentY);
 for(std::size_t i = 0; i < packs.size(); ++i) {
  const auto pack = packs[i];
  const int row = static_cast<int>((i + 1) / 2);
  const int col = static_cast<int>((i + 1) % 2);
  const int widgetY = contentY + row * layout::kRowSpacing;
  addActionButton(layout::optionsGridX(width(), col),
                  listTop + widgetY,
                  kButtonWidth,
                  kButtonHeight,
                  pack.name + (pack.valid ? "" : " (invalid)") + (pack.selected ? " (selected)" : ""),
                  [this, key = pack.key] {
                   if(minecraft() != nullptr && minecraft()->gameRenderer != nullptr &&
                      minecraft()->gameRenderer->shaderPacks() != nullptr) {
                    minecraft()->gameRenderer->shaderPacks()->select(key);
                    rebuildLayout();
                   }
                  });
  scroll_.addEntry(static_cast<int>(buttons_.size() - 1), widgetY);
 }
 contentY += static_cast<int>((packs.size() + 2) / 2) * layout::kRowSpacing +
             layout::OptionsListScroll::kSectionGap;
 const render::PackDefinition* manifest = manager->selectedDefinition();
 if(manifest != nullptr && !manifest->settings.empty()) {
  const std::vector<std::string>* layoutTokens = nullptr;
  if(!currentPage_.empty()) {
   const auto page = manifest->screenPages.find(currentPage_);
   if(page != manifest->screenPages.end()) layoutTokens = &page->second;
  } else if(!manifest->screenRoot.empty()) {
   layoutTokens = &manifest->screenRoot;
  }
  std::vector<const PackSetting*> visible;
  visible.reserve(manifest->settings.size());
  auto findSetting = [&](const std::string& key) -> const PackSetting* {
   for(const PackSetting& setting : manifest->settings) {
    if(setting.key == key) return &setting;
   }
   return nullptr;
  };
  if(layoutTokens != nullptr) {
   scroll_.addHeader(currentPage_.empty() ? "Shader Options" : currentPage_, contentY);
   contentY += layout::OptionsListScroll::kSectionLabelHeight;
   if(!currentPage_.empty()) {
    addActionButton(layout::optionsGridX(width(), 0),
                    listTop + contentY,
                    kButtonWidth,
                    kButtonHeight,
                    "< Back",
                    [this] {
                     currentPage_.clear();
                     rebuildLayout();
                    });
    scroll_.addEntry(static_cast<int>(buttons_.size() - 1), contentY);
    contentY += layout::kRowSpacing;
   }
   if(!manifest->profiles.empty() &&
      std::find(layoutTokens->begin(), layoutTokens->end(), "<profile>") != layoutTokens->end()) {
    for(std::size_t pi = 0; pi < manifest->profiles.size(); ++pi) {
     const render::PackProfile profile = manifest->profiles[pi];
     const int col = static_cast<int>(pi % 2);
     const int row = static_cast<int>(pi / 2);
     const int widgetY = contentY + row * layout::kRowSpacing;
     addActionButton(layout::optionsGridX(width(), col),
                     listTop + widgetY,
                     kButtonWidth,
                     kButtonHeight,
                     "Profile: " + profile.name,
                     [this, profile] {
                      if(minecraft() == nullptr || minecraft()->gameRenderer == nullptr ||
                         minecraft()->gameRenderer->shaderPacks() == nullptr) {
                       return;
                      }
                      std::vector<std::pair<std::string, std::string>> values;
                      values.reserve(profile.values.size());
                      for(const auto& [key, value] : profile.values) {
                       values.emplace_back(key, value);
                      }
                      // One atomic rebuild: per-option setSetting calls made a
                      // profile click restart the whole pack compile once per
                      // option and freeze the game.
                      minecraft()->gameRenderer->shaderPacks()->setSettings(values);
                      rebuildLayout();
                     });
     scroll_.addEntry(static_cast<int>(buttons_.size() - 1), widgetY);
    }
    contentY += static_cast<int>((manifest->profiles.size() + 1) / 2) * layout::kRowSpacing;
   }
   std::size_t optionSlot = 0;
   for(const std::string& token : *layoutTokens) {
    if(token == "<empty>" || token == "<profile>") continue;
    if(!token.empty() && token.front() == '[' && token.back() == ']') {
     const std::string page = token.substr(1, token.size() - 2);
     const int col = static_cast<int>(optionSlot % 2);
     const int row = static_cast<int>(optionSlot / 2);
     const int widgetY = contentY + row * layout::kRowSpacing;
     addActionButton(layout::optionsGridX(width(), col),
                     listTop + widgetY,
                     kButtonWidth,
                     kButtonHeight,
                     page + " >",
                     [this, page] {
                      currentPage_ = page;
                      rebuildLayout();
                     });
     scroll_.addEntry(static_cast<int>(buttons_.size() - 1), widgetY);
     ++optionSlot;
     continue;
    }
    if(token == "*") {
     for(const PackSetting& setting : manifest->settings) visible.push_back(&setting);
     continue;
    }
    if(const PackSetting* setting = findSetting(token)) visible.push_back(setting);
   }
   contentY += static_cast<int>((optionSlot + 1) / 2) * layout::kRowSpacing;
  } else {
   scroll_.addHeader("Shader Options", contentY);
   contentY += layout::OptionsListScroll::kSectionLabelHeight;
   for(const PackSetting& setting : manifest->settings) visible.push_back(&setting);
  }
  const int optionsBase = contentY;
  for(std::size_t i = 0; i < visible.size(); ++i) {
   const PackSetting setting = *visible[i];
   const int row = static_cast<int>(i / 2);
   const int col = static_cast<int>(i % 2);
   const int widgetY = optionsBase + row * layout::kRowSpacing;
   const int widgetIndex = static_cast<int>(buttons_.size());
   const int x = layout::optionsGridX(width(), col);
   const bool useSlider = setting.asSlider || setting.type != SettingType::Bool;
   if(!useSlider) {
    addActionButton(x,
                    listTop + widgetY,
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
        listTop + widgetY,
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
   scroll_.addEntry(widgetIndex, widgetY);
  }
  contentY = optionsBase + static_cast<int>((visible.size() + 1) / 2) * layout::kRowSpacing +
             layout::OptionsListScroll::kSectionGap;
 }
 const int contentHeight = std::max(0, contentY - layout::OptionsListScroll::kSectionGap);
 const int doneY = height() - 28;
 scroll_.setViewport(listTop, std::max(listTop, doneY - layout::OptionsListScroll::kSectionGap));
 scroll_.setContentHeight(contentHeight);
 addActionButton(width() / 2 - kButtonWidth / 2,
                 doneY,
                 kButtonWidth,
                 kButtonHeight,
                 "Done",
                 [this] { navigateTo(parentFactory_); });
 updateListLayout();
}
void ShaderpackScreen::updateListLayout() {
 scroll_.apply(buttons_);
}
void ShaderpackScreen::mouseScrolled(int mouseX, int mouseY, int delta) {
 (void)mouseX;
 if(scroll_.mouseScrolled(mouseY, delta)) {
  updateListLayout();
 }
}
void ShaderpackScreen::mouseClicked(int mouseX, int mouseY, int button) {
 if(button == 0 && scroll_.mouseClicked(mouseX, mouseY, width())) {
  updateListLayout();
  return;
 }
 Screen::mouseClicked(mouseX, mouseY, button);
}
void ShaderpackScreen::mouseReleased(int mouseX, int mouseY, int button) {
 scroll_.mouseReleased();
 Screen::mouseReleased(mouseX, mouseY, button);
}
void ShaderpackScreen::keyPressed(char character, int keyCode) {
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
  navigateTo(parentFactory_);
  return;
 }
 Screen::keyPressed(character, keyCode);
}
void ShaderpackScreen::render(int mouseX, int mouseY, float tickDelta) {
 if(scroll_.draggingThumb()) {
  scroll_.mouseDragged(mouseY);
  updateListLayout();
 }
 renderBackground();
 scroll_.renderFooterDim(width(), height());
 if(textRenderer() != nullptr) {
  textRenderer()->drawCenteredWithShadow(title_, width() / 2, 12, 0xFFFFFF);
 }
 scroll_.renderHeaders(textRenderer(), width());
 scroll_.renderScrollbar(width());
 Screen::render(mouseX, mouseY, tickDelta);
}
} // namespace net::minecraft::client::gui::screen::pack
