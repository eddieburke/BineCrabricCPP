#pragma once
#include <memory>
#include <string>
#include "net/minecraft/client/gui/screen/Screen.hpp"
#include "net/minecraft/client/gui/widget/ActionButtonWidget.hpp"
#include "net/minecraft/client/gui/widget/TextFieldWidget.hpp"
#include "net/minecraft/client/host/ServerProcessCoordinator.hpp"
namespace net::minecraft::client::host {
class LanScreen : public gui::screen::Screen {
 public:
 LanScreen(std::string errorMessage = {}, std::string portText = "25565");
 void init() override;
 void tick() override;
 void removed() override;
 void render(int mouseX, int mouseY, float tickDelta) override;

 protected:
 void keyPressed(char character, int keyCode) override;
 void mouseClicked(int mouseX, int mouseY, int button) override;
 [[nodiscard]] std::string_view getScreenUiId() const override {
  return net::minecraft::mod::screen_ids::kLan;
 }

 private:
 void updateOpenButtonState();
 void backToGameMenu();
 void openLan();
 void pollServerStart();
 void refreshSettingLabels();
 std::unique_ptr<gui::widget::TextFieldWidget> portField_;
 gui::widget::ActionButtonWidget* openButton_ = nullptr;
 gui::widget::ActionButtonWidget* pvpButton_ = nullptr;
 gui::widget::ActionButtonWidget* animalsButton_ = nullptr;
 gui::widget::ActionButtonWidget* netherButton_ = nullptr;
 gui::widget::ActionButtonWidget* modsButton_ = nullptr;
 ServerProcessSettings settings_;
 bool startingServer_ = false;
 std::string errorMessage_;
 std::string portText_;
};
} // namespace net::minecraft::client::host
