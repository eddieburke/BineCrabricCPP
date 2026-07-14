#pragma once
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include "net/minecraft/client/gui/layout/ScreenLayout.hpp"
#include "net/minecraft/client/gui/screen/Screen.hpp"
#include "net/minecraft/client/gui/widget/ActionButtonWidget.hpp"
#include "net/minecraft/client/gui/widget/OptionButtonWidget.hpp"
#include "net/minecraft/client/option/OptionRegistry.hpp"
#include "net/minecraft/client/option/OptionSpec.hpp"
namespace net::minecraft::client {
class Minecraft;
}
namespace net::minecraft::client::option {
class GameOptions;
}
namespace net::minecraft::client::gui::layout {
struct OptionsBuildContext {
  screen::Screen& screen;
  client::Minecraft& minecraft;
  option::GameOptions& gameOptions;
};
inline void refreshOptionStates(const std::vector<std::unique_ptr<widget::ButtonWidget>>& buttons,
                                const option::GameOptions& gameOptions) {
  for(const std::unique_ptr<widget::ButtonWidget>& btnPtr : buttons) {
    if(btnPtr == nullptr) {
      continue;
    }
    const std::optional<std::size_t> registryIndex = btnPtr->getRegistryIndex();
    if(!registryIndex) {
      continue;
    }
    const option::OptionSpec& spec = option::OptionRegistry::at(*registryIndex);
    if(spec.isEnabled != nullptr) {
      btnPtr->active = spec.isEnabled(gameOptions);
    }
  }
}
inline widget::ActionButtonWidget& addDoneButton(OptionsBuildContext& ctx,
                                                 int y,
                                                 std::string text,
                                                 std::function<void()> onDone) {
  return ctx.screen.addActionButton(centerBtnX(ctx.screen.width()),
                                    y,
                                    kDefaultButtonWidth,
                                    kDefaultButtonHeight,
                                    std::move(text),
                                    std::move(onDone));
}
inline bool handleOptionWidgetClick(widget::ButtonWidget& button, OptionsBuildContext& ctx) {
  if(!button.active) {
    return false;
  }
  const std::optional<std::size_t> registryIndex = button.getRegistryIndex();
  if(!registryIndex) {
    return false;
  }
  const option::OptionSpec& spec = option::OptionRegistry::at(*registryIndex);
  if(spec.kind == option::OptionSpec::Kind::Slider) {
    return false;
  }
  if(spec.cycle != nullptr) {
    ctx.gameOptions.cycle(spec.persistKey, 1);
    if(auto* optBtn = dynamic_cast<widget::OptionButtonWidget*>(&button)) {
      optBtn->refreshText(ctx.gameOptions);
    }
    if(spec.needsScreenResize != nullptr && spec.needsScreenResize(ctx.gameOptions)) {
      ctx.minecraft.scheduleScreenResize();
    }
    return true;
  }
  return false;
}
} // namespace net::minecraft::client::gui::layout
