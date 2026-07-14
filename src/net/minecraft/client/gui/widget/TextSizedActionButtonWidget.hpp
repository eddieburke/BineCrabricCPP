#pragma once
#include <functional>
#include <string>
#include "net/minecraft/client/gui/widget/ActionButtonWidget.hpp"
namespace net::minecraft::client::font {
class TextRenderer;
}
namespace net::minecraft::client::gui::widget {
// Action button sized to its label. The left cap is sampled from u=2 in gui.png so the
// outer drop shadow is not clipped on narrow buttons; width includes kLeftCrop padding.
class TextSizedActionButtonWidget : public ActionButtonWidget {
public:
  static constexpr int kLeftCrop = 2;
  static constexpr int kHorizontalPad = 8;
  [[nodiscard]] static int measureWidth(const font::TextRenderer& textRenderer, const std::string& text);
  TextSizedActionButtonWidget(int x, int y, std::string text, std::function<void()> onClick)
      : TextSizedActionButtonWidget(x, y, 200, 20, std::move(text), std::move(onClick)) {
  }
  TextSizedActionButtonWidget(int x, int y, int width, int height, std::string text, std::function<void()> onClick)
      : ActionButtonWidget(x, y, width, height, std::move(text), std::move(onClick)) {
  }
  void render(client::Minecraft& minecraft, font::TextRenderer& textRenderer, int mouseX, int mouseY) override;
};
} // namespace net::minecraft::client::gui::widget
