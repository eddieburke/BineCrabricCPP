#pragma once
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/gl/GL11.hpp"
#include "net/minecraft/client/gui/widget/ButtonWidget.hpp"
#include "net/minecraft/client/option/GameOptions.hpp"
#include "net/minecraft/client/option/OptionRegistry.hpp"
#include <algorithm>
#include <cstddef>
#include <functional>
#include <string>
namespace net::minecraft::client::gui::widget {
class SliderWidget : public ButtonWidget {
public:
  using TextFormat = std::function<std::string(const option::GameOptions&)>;
  float value = 1.0f;
  bool dragging = false;
  SliderWidget(int id, int x, int y, std::size_t registryIndex, client::Minecraft& minecraft, std::string text,
               float valueIn, TextFormat format)
      : ButtonWidget(id, x, y, 150, 20, std::move(text)), value(std::clamp(valueIn, 0.0f, 1.0f)),
        registryIndex_(registryIndex), minecraft_(&minecraft), format_(std::move(format)) {
  }
  [[nodiscard]] std::optional<std::size_t> getRegistryIndex() const override {
    return registryIndex_;
  }
  void refreshText(const option::GameOptions& options) {
    if(format_) {
      text = format_(options);
    }
  }
  [[nodiscard]] int getYImage(bool /*hovered*/) const override {
    return 0;
  }
  void renderBackground(int mouseX, int /*mouseY*/) override {
    if(!visible || minecraft_ == nullptr) {
      return;
    }
    if(dragging) {
      updateValue(mouseX);
    }
    gl::GL11::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    const int knobX = x + static_cast<int>(value * static_cast<float>(width - 8));
    drawTexture(knobX, y, 0, 66, 4, 20);
    drawTexture(knobX + 4, y, 196, 66, 4, 20);
  }
  [[nodiscard]] bool isMouseOver(int mouseX, int mouseY) const override {
    if(!ButtonWidget::isMouseOver(mouseX, mouseY)) {
      return false;
    }
    const_cast<SliderWidget*>(this)->updateValue(mouseX);
    const_cast<SliderWidget*>(this)->dragging = true;
    return true;
  }
  void mouseReleased(int mouseX, int mouseY) override {
    (void)mouseX;
    (void)mouseY;
    dragging = false;
  }

private:
  void updateValue(int mouseX) {
    if(minecraft_ == nullptr) {
      return;
    }
    value = std::clamp(static_cast<float>(mouseX - (x + 4)) / static_cast<float>(width - 8), 0.0f, 1.0f);
    const option::OptionSpec& spec = option::OptionRegistry::at(registryIndex_);
    minecraft_->options.setFloat(spec.persistKey, value);
    refreshText(minecraft_->options);
  }
  std::size_t registryIndex_ = 0;
  client::Minecraft* minecraft_ = nullptr;
  TextFormat format_;
};
} // namespace net::minecraft::client::gui::widget
