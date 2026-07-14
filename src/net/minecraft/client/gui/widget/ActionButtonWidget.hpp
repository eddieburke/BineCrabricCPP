#pragma once
#include <functional>
#include <string>
#include <utility>
#include "net/minecraft/client/gui/widget/ButtonWidget.hpp"
namespace net::minecraft::client::gui::widget {
class ActionButtonWidget : public ButtonWidget {
public:
  static constexpr int kNoId = -1;
  ActionButtonWidget(int x, int y, std::string text, std::function<void()> onClick)
      : ActionButtonWidget(kNoId, x, y, 200, 20, std::move(text), std::move(onClick)) {
  }
  ActionButtonWidget(int x, int y, int width, int height, std::string text, std::function<void()> onClick)
      : ActionButtonWidget(kNoId, x, y, width, height, std::move(text), std::move(onClick)) {
  }
  ActionButtonWidget(int id, int x, int y, int width, int height, std::string text, std::function<void()> onClick)
      : ButtonWidget(id, x, y, width, height, std::move(text)), onClick(std::move(onClick)) {
  }
  std::function<void()> onClick;
};
} // namespace net::minecraft::client::gui::widget
