#pragma once
// Faithful port of gui.widget.TextFieldWidget (beta 1.7.3 MCP).
#include "net/minecraft/client/gui/DrawContext.hpp"
#include <string>
namespace net::minecraft::client::font {
class TextRenderer;
}
namespace net::minecraft::client::gui::screen {
class Screen;
}
namespace net::minecraft::client::gui::widget {
class TextFieldWidget : public gui::DrawContext {
public:
  TextFieldWidget(screen::Screen* parent, font::TextRenderer* textRenderer, int x, int y, int width, int height,
                  std::string text);
  void setText(const std::string& text);
  [[nodiscard]] const std::string& getText() const {
    return text_;
  }
  void tick();
  void keyPressed(char character, int keyCode);
  void mouseClicked(int mouseX, int mouseY, int button);
  void setFocused(bool focused);
  void render();
  void setMaxLength(int maxLength);
  void setBounds(int x, int y, int width, int height);
  bool focused = false;
  bool enabled = true;

private:
  screen::Screen* parent_ = nullptr;
  font::TextRenderer* textRenderer_ = nullptr;
  int x_ = 0;
  int y_ = 0;
  int width_ = 0;
  int height_ = 0;
  std::string text_;
  int maxLength_ = 0;
  int focusedTicks_ = 0;
};
} // namespace net::minecraft::client::gui::widget
