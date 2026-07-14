#pragma once
#include "net/minecraft/client/gui/screen/Screen.hpp"
#include "net/minecraft/screen/ScreenHandler.hpp"
namespace net::minecraft::client::gui::screen::ingame {
class HandledScreen : public screen::Screen {
public:
  explicit HandledScreen(::net::minecraft::screen::ScreenHandler* container) : container_(container) {
  }
  void init() override;
  void render(int mouseX, int mouseY, float tickDelta) override;
  void mouseClicked(int mouseX, int mouseY, int button) override;
  void keyPressed(char character, int keyCode) override;
  void removed() override;
  void tick() override;
  [[nodiscard]] bool shouldPause() const override {
    return false;
  }

protected:
  virtual void drawForeground();
  virtual void drawBackground(float tickDelta) = 0;
  [[nodiscard]] int containerOriginX() const noexcept;
  [[nodiscard]] int containerOriginY() const noexcept;
  void drawContainerTexture(const char* texturePath, int srcU, int srcV, int drawW, int drawH);
  void drawContainerTextureSplit(const char* texturePath, int topDrawH, int bottomSrcV, int bottomDrawH);
  int backgroundWidth = 176;
  int backgroundHeight = 166;
  ::net::minecraft::screen::ScreenHandler* container_ = nullptr;

private:
  void drawSlot(const ::net::minecraft::screen::slot::Slot& slot);
  ::net::minecraft::screen::slot::Slot* getSlotAt(int x, int y);
  [[nodiscard]] bool isPointOverSlot(const ::net::minecraft::screen::slot::Slot& slot, int x, int y) const;
};
} // namespace net::minecraft::client::gui::screen::ingame
