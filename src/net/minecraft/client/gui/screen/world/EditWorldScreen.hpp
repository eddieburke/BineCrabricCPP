#pragma once
#include <memory>
#include <string>
#include "net/minecraft/client/gui/screen/Screen.hpp"
#include "net/minecraft/client/gui/widget/TextFieldWidget.hpp"
namespace net::minecraft::client::gui::screen::world {
class EditWorldScreen : public screen::Screen {
public:
  EditWorldScreen(screen::ScreenFactory parentFactory, std::string worldSaveName);
  void init() override;
  void tick() override;
  void removed() override;
  void render(int mouseX, int mouseY, float tickDelta) override;
  void keyPressed(char character, int keyCode) override;
  void mouseClicked(int mouseX, int mouseY, int button) override;
  [[nodiscard]] std::string_view getScreenUiId() const override {
    return net::minecraft::mod::screen_ids::kEditWorld;
  }

private:
  screen::ScreenFactory parentFactory_;
  std::string worldSaveName_;
  void renameWorld();
  void updateRenameButtonState();
  std::unique_ptr<widget::TextFieldWidget> levelNameField_;
  widget::ActionButtonWidget* renameButton_ = nullptr;
};
} // namespace net::minecraft::client::gui::screen::world
