#include "net/minecraft/client/gui/screen/pack/ShaderpackScreen.hpp"
#include <algorithm>
#include <cstdlib>
#include <iomanip>
#include <sstream>
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/gui/layout/OptionsLayout.hpp"
#include "net/minecraft/client/gui/layout/ScreenLayout.hpp"
#include "net/minecraft/client/render/GameRenderer.hpp"
#include "net/minecraft/client/render/shaderpack/ShaderPackManager.hpp"
namespace net::minecraft::client::gui::screen::pack {
namespace shaderpack = net::minecraft::client::render::shaderpack;
namespace {
using shaderpack::PackSetting;
using shaderpack::SettingType;
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
 buttons_.clear();
 title_ = "Shaderpacks";
 if(minecraft() == nullptr || minecraft()->gameRenderer == nullptr ||
    minecraft()->gameRenderer->shaderPacks() == nullptr) {
  return;
 }
 auto* manager = minecraft()->gameRenderer->shaderPacks();
 const auto packs = manager->available();
 const int startY = height() / 6;
 addActionButton(layout::optionsGridX(width(), 0), startY, 150, layout::kDefaultButtonHeight,
                 std::string("None") + (manager->active() ? "" : " (selected)"), [this] {
                  if(minecraft() != nullptr && minecraft()->gameRenderer != nullptr &&
                     minecraft()->gameRenderer->shaderPacks() != nullptr) {
                   minecraft()->gameRenderer->shaderPacks()->select("");
                   init();
                  }
                 });
 for(std::size_t i = 0; i < packs.size(); ++i) {
  const auto pack = packs[i];
  addActionButton(layout::optionsGridX(width(), static_cast<int>((i + 1) % 2)),
                  startY + static_cast<int>((i + 1) / 2) * layout::kRowSpacing,
                  150,
                  layout::kDefaultButtonHeight,
                  pack.name + (pack.valid ? "" : " (invalid)"),
                  [this, key = pack.key] {
                   if(minecraft() != nullptr && minecraft()->gameRenderer != nullptr &&
                      minecraft()->gameRenderer->shaderPacks() != nullptr) {
                    minecraft()->gameRenderer->shaderPacks()->select(key);
                    init();
                   }
                  });
 }
 const shaderpack::PackManifest* manifest = manager->activeManifest();
 const int settingsStart = startY + ((static_cast<int>(packs.size()) + 2) / 2) * layout::kRowSpacing;
 if(manifest != nullptr) {
  for(std::size_t i = 0; i < manifest->settings.size(); ++i) {
   const PackSetting setting = manifest->settings[i];
   addActionButton(layout::optionsGridX(width(), static_cast<int>(i % 2)),
                   settingsStart + static_cast<int>(i / 2) * layout::kRowSpacing,
                   150,
                   layout::kDefaultButtonHeight,
                   settingLabel(setting, manager->settingValue(setting.key)),
                   [this, setting] {
                    if(minecraft() == nullptr || minecraft()->gameRenderer == nullptr ||
                       minecraft()->gameRenderer->shaderPacks() == nullptr) {
                     return;
                    }
                    auto* current = minecraft()->gameRenderer->shaderPacks();
                    const std::string oldValue = current->settingValue(setting.key);
                    std::string next = oldValue;
                    if(setting.type == shaderpack::SettingType::Bool) {
                     next = oldValue == "0" ? "1" : "0";
                    } else {
                     double value = std::strtod(oldValue.c_str(), nullptr) + setting.step;
                     if(value > setting.maximum) {
                      value = setting.minimum;
                     }
                     next = setting.type == shaderpack::SettingType::Int
                                ? std::to_string(static_cast<int>(value))
                                : std::to_string(value);
                    }
                    current->setSetting(setting.key, next);
                    init();
                   });
  }
 }
 const int doneY = std::min(height() - 48, settingsStart +
                                               static_cast<int>((manifest == nullptr ? 0 : manifest->settings.size() + 1) / 2) * layout::kRowSpacing + 8);
 addCenteredActionButton(doneY, "Done", [this] { navigateTo(parentFactory_); });
}
void ShaderpackScreen::render(int mouseX, int mouseY, float tickDelta) {
 renderBackground();
 if(textRenderer() != nullptr) {
  drawCenteredTextWithShadow(*textRenderer(), title_, width() / 2, 20, 0xFFFFFF);
 }
 Screen::render(mouseX, mouseY, tickDelta);
}
} // namespace net::minecraft::client::gui::screen::pack
