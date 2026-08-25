#pragma once
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include "net/minecraft/client/gui/layout/OptionsListScroll.hpp"
#include "net/minecraft/client/gui/screen/Screen.hpp"
#include "net/minecraft/client/option/GameOptions.hpp"
namespace net::minecraft::client::gui::screen::option {
namespace client_option = net::minecraft::client::option;
class KeybindsScreen : public screen::Screen {
 public:
 using ParentFactory = std::function<std::unique_ptr<screen::Screen>()>;
 KeybindsScreen(ParentFactory parentFactory, client_option::GameOptions* gameOptions);
 void init() override;
 void render(int mouseX, int mouseY, float tickDelta) override;
 void keyPressed(char character, int keyCode) override;
 void mouseClicked(int mouseX, int mouseY, int button) override;
 void mouseReleased(int mouseX, int mouseY, int button) override;
 void mouseScrolled(int mouseX, int mouseY, int delta) override;
 [[nodiscard]] std::string_view getScreenUiId() const override {
  return net::minecraft::mod::screen_ids::kKeybinds;
 }

 private:
 struct BindingEntry {
  enum class Source { Vanilla,
                      Mod } source = Source::Vanilla;
  int vanillaIndex = -1;
  std::string modId;
  int buttonIndex = -1;
 };
 void selectKeybind(int index);
 void updateListLayout();
 [[nodiscard]] std::string bindingName(const BindingEntry& binding) const;
 [[nodiscard]] std::string bindingKey(const BindingEntry& binding) const;
 [[nodiscard]] std::string bindingText(const BindingEntry& binding, bool selected) const;
 ParentFactory parentFactory_;
 client_option::GameOptions* gameOptions_ = nullptr;
 std::string title_;
 int selectedKeyBinding_ = -1;
 std::vector<BindingEntry> bindings_;
 layout::OptionsListScroll scroll_;
};
} // namespace net::minecraft::client::gui::screen::option
