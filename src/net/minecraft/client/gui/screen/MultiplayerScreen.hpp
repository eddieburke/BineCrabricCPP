#pragma once
#include "net/minecraft/client/gui/screen/Screen.hpp"
#include "net/minecraft/client/gui/widget/ActionButtonWidget.hpp"
#include "net/minecraft/client/gui/widget/TextFieldWidget.hpp"
#include <memory>
namespace net::minecraft::client::gui::screen {
class MultiplayerScreen : public Screen {
public:
  explicit MultiplayerScreen(ScreenFactory parentFactory = {});
  void init() override;
  void tick() override;
  void removed() override;
  void render(int mouseX, int mouseY, float tickDelta) override;

protected:
  void keyPressed(char character, int keyCode) override;
  void mouseClicked(int mouseX, int mouseY, int button) override;
  [[nodiscard]] std::string_view getScreenUiId() const override { return net::minecraft::mod::screen_ids::kMultiplayer; }

 private:
  [[nodiscard]] static int parseInt(const std::string& value, int defaultValue);
  void updateConnectButtonState();
  void connectToServer();
  ScreenFactory parentFactory_;
  std::unique_ptr<widget::TextFieldWidget> serverField_;
  widget::ActionButtonWidget* connectButton_ = nullptr;
  widget::ActionButtonWidget* modsToggle_ = nullptr;
  void toggleMods();
};
} // namespace net::minecraft::client::gui::screen
