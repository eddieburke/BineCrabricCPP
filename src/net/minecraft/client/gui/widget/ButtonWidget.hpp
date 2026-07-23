#pragma once
#include <cstddef>
#include <optional>
#include <string>
#include "net/minecraft/client/gui/DrawContext.hpp"
namespace net::minecraft::client {
class Minecraft;
}
namespace net::minecraft::client::font {
class TextRenderer;
}
namespace net::minecraft::client::gui::widget {
class ButtonWidget : public gui::DrawContext {
 public:
 ButtonWidget() = default;
 ButtonWidget(int idIn, int xIn, int yIn, std::string textIn)
     : ButtonWidget(idIn, xIn, yIn, 200, 20, std::move(textIn)) {
 }
 ButtonWidget(int idIn, int xIn, int yIn, int widthIn, int heightIn, std::string textIn);
 virtual ~ButtonWidget() = default;
 virtual int getYImage(bool hovered) const;
 virtual void render(client::Minecraft& minecraft, font::TextRenderer& textRenderer, int mouseX, int mouseY);
 virtual void renderBackground(int mouseX, int mouseY);
 virtual void onMouseDown(int mouseX, int mouseY);
 virtual void mouseReleased(int mouseX, int mouseY);
 [[nodiscard]] virtual bool isMouseOver(int mouseX, int mouseY) const;
 [[nodiscard]] virtual std::optional<std::size_t> getRegistryIndex() const {
  return std::nullopt;
 }
 int width = 200;
 int height = 20;
 int x = 0;
 int y = 0;
 std::string text{};
 int id = 0;
 bool active = true;
 bool visible = true;
};
} // namespace net::minecraft::client::gui::widget
