#include "net/minecraft/mod/ScreenUi.hpp"
#include "net/minecraft/client/gui/layout/ScreenLayout.hpp"
#include "net/minecraft/client/gui/screen/Screen.hpp"
#include "net/minecraft/client/gui/widget/ActionButtonWidget.hpp"
namespace net::minecraft::mod {
client::gui::widget::ActionButtonWidget& ScreenUiContext::addCenteredButton(int y,
                                                                            std::string text,
                                                                            std::function<void()> onClick) const {
  return screen->addCenteredActionButton(y, std::move(text), std::move(onClick));
}
client::gui::widget::ActionButtonWidget& ScreenUiContext::addButton(
    int x, int y, int width, int height, std::string text, std::function<void()> onClick) const {
  return screen->addActionButton(x, y, width, height, std::move(text), std::move(onClick));
}
client::gui::widget::ActionButtonWidget& ScreenUiContext::addStackedCenteredButton(
    std::string text, std::function<void()> onClick) const {
  client::gui::widget::ActionButtonWidget& button =
      screen->addCenteredActionButton(*stackedButtonY, std::move(text), std::move(onClick));
  *stackedButtonY += client::gui::layout::kRowSpacing;
  return button;
}
} // namespace net::minecraft::mod
