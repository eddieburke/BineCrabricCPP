#pragma once
#include <functional>
#include <initializer_list>
#include <optional>
#include <string>
#include <vector>
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/gui/layout/OptionsLayout.hpp"
#include "net/minecraft/client/gui/layout/ScreenLayout.hpp"
#include "net/minecraft/client/gui/screen/Screen.hpp"
#include "net/minecraft/client/gui/widget/ActionButtonWidget.hpp"
#include "net/minecraft/client/gui/widget/OptionButtonWidget.hpp"
#include "net/minecraft/client/gui/widget/SliderWidget.hpp"
#include "net/minecraft/client/option/GameOptions.hpp"
#include "net/minecraft/client/option/OptionRegistry.hpp"
#include "net/minecraft/client/resource/language/I18n.hpp"
namespace net::minecraft::client::gui::screen::option {
namespace client_option = net::minecraft::client::option;
inline std::string optionLabel(const char* title, const char* value) {
 return std::string(title) + ": " + value;
}
inline std::string optionLabel(const char* title, const std::string& value) {
 return std::string(title) + ": " + value;
}
inline std::string percentLabel(const char* title, float value) {
 if(value == 0.0f) {
  return optionLabel(title, resource::language::I18n::getTranslation("options.off"));
 }
 return optionLabel(title, std::to_string(static_cast<int>(value * 100.0f)) + "%");
}
inline widget::OptionButtonWidget::TextFormat cycleLabelFormat(
    std::string title, std::vector<std::string> labels, int client_option::GameOptions::* member, bool translate) {
 return [title = std::move(title), labels = std::move(labels), member, translate](const client_option::GameOptions& o) {
  if(labels.empty()) return title;
  const int count = static_cast<int>(labels.size());
  const int index = ((o.*member % count) + count) % count;
  const std::string& label = labels[static_cast<std::size_t>(index)];
  return optionLabel(title.c_str(), translate ? resource::language::I18n::getTranslation(label) : label);
 };
}
inline void refreshOptionLabels(screen::Screen& screen, const client_option::GameOptions& options) {
 for(const std::unique_ptr<widget::ButtonWidget>& btnPtr : screen.buttons()) {
  if(btnPtr == nullptr) {
   continue;
  }
  if(auto* optBtn = dynamic_cast<widget::OptionButtonWidget*>(btnPtr.get())) {
   optBtn->refreshText(options);
  } else if(auto* slider = dynamic_cast<widget::SliderWidget*>(btnPtr.get())) {
   slider->refreshText(options);
  }
 }
}
inline bool handleOptionButtonClick(screen::Screen& screen, widget::ButtonWidget& button) {
 client::Minecraft* mc = screen.minecraft();
 if(mc == nullptr || !button.active || !button.getRegistryIndex()) {
  return false;
 }
 layout::OptionsBuildContext ctx{screen, *mc, mc->options};
 if(!layout::handleOptionWidgetClick(button, ctx)) {
  return false;
 }
 layout::refreshOptionStates(screen.buttons(), mc->options);
 refreshOptionLabels(screen, mc->options);
 return true;
}
class OptionGuiBuilder {
 public:
 OptionGuiBuilder(screen::Screen& screen, client::Minecraft& minecraft, client_option::GameOptions& options)
     : screen_(screen), minecraft_(minecraft), options_(options) {
 }
 [[nodiscard]] int gridX(int column) const {
  return layout::optionsGridX(screen_.width(), column);
 }
 [[nodiscard]] int gridY(int row) const {
  return layout::optionsGridY(screen_.height(), row);
 }
 void toggle(int x,
             int y,
             const char* key,
             const char* title,
             bool (*isEnabled)(const client_option::GameOptions&) = nullptr) {
  auto format = [title = std::string(title), key = std::string(key)](const client_option::GameOptions& o) {
   const bool on = o.getBoolean(key);
   return optionLabel(title.c_str(),
                      resource::language::I18n::getTranslation(on ? "options.on" : "options.off"));
  };
  customCycle(x, y, key, std::move(format), isEnabled);
 }
 void boolLabels(int x,
                 int y,
                 const char* key,
                 const char* title,
                 const char* on,
                 const char* off,
                 bool (*isEnabled)(const client_option::GameOptions&) = nullptr) {
  auto format = [title = std::string(title),
                 on = std::string(on),
                 off = std::string(off),
                 key = std::string(key)](const client_option::GameOptions& o) {
   return optionLabel(title.c_str(), o.getBoolean(key) ? on.c_str() : off.c_str());
  };
  customCycle(x, y, key, std::move(format), isEnabled);
 }
 void intCycle(int x,
               int y,
               const char* key,
               const char* title,
               std::initializer_list<const char*> labels,
               int client_option::GameOptions::* member,
               bool (*isEnabled)(const client_option::GameOptions&) = nullptr) {
  customCycle(x, y, key, cycleLabelFormat(title, std::vector<std::string>(labels.begin(), labels.end()),
                                        member, false), isEnabled);
 }
 void i18nCycle(int x,
                int y,
                const char* key,
                const char* title,
                std::initializer_list<const char*> i18nKeys,
                int client_option::GameOptions::* member,
                bool (*isEnabled)(const client_option::GameOptions&) = nullptr) {
  customCycle(x, y, key, cycleLabelFormat(title, std::vector<std::string>(i18nKeys.begin(), i18nKeys.end()),
                                        member, true), isEnabled);
 }
 void customCycle(int x,
                  int y,
                  const char* key,
                  std::function<std::string(const client_option::GameOptions&)> format,
                  bool (*isEnabled)(const client_option::GameOptions&) = nullptr) {
  const std::optional<std::size_t> idx = client_option::OptionRegistry::indexOf(key);
  if(!idx) {
   return;
  }
  widget::OptionButtonWidget& btn = screen_.addButton<widget::OptionButtonWidget>(
      widget::ActionButtonWidget::kNoId, x, y, *idx, format(options_), std::move(format));
  if(isEnabled != nullptr) {
   btn.active = isEnabled(options_);
  }
 }
 void slider(int x,
             int y,
             const char* key,
             const char* title,
             std::function<std::string(const client_option::GameOptions&)> format = nullptr,
             bool (*isEnabled)(const client_option::GameOptions&) = nullptr) {
  if(!format) {
   format = [title = std::string(title), key = std::string(key)](const client_option::GameOptions& o) {
    return percentLabel(title.c_str(), o.getFloat(key));
   };
  }
  mappedSlider(x, y, key, [key = std::string(key)](const client_option::GameOptions& o) { return o.getFloat(key); },
                std::move(format), isEnabled);
 }
 void mappedSlider(int x,
                   int y,
                   const char* key,
                   std::function<float(const client_option::GameOptions&)> getValue,
                   std::function<std::string(const client_option::GameOptions&)> format,
                   bool (*isEnabled)(const client_option::GameOptions&) = nullptr) {
  const std::optional<std::size_t> idx = client_option::OptionRegistry::indexOf(key);
  if(!idx) {
   return;
  }
  widget::SliderWidget& btn = screen_.addButton<widget::SliderWidget>(widget::ActionButtonWidget::kNoId,
                                                                      x,
                                                                      y,
                                                                      *idx,
                                                                      minecraft_,
                                                                      format(options_),
                                                                      getValue(options_),
                                                                      std::move(format));
  if(isEnabled != nullptr) {
   btn.active = isEnabled(options_);
  }
 }

 private:
 screen::Screen& screen_;
 client::Minecraft& minecraft_;
 client_option::GameOptions& options_;
};
} // namespace net::minecraft::client::gui::screen::option
