#pragma once
#include "net/minecraft/client/gui/screen/Screen.hpp"
namespace net::minecraft::client::gui::screen {
class FatalErrorScreen : public Screen {
 public:
 void init() override {
 }
 [[nodiscard]] std::string_view getScreenUiId() const override {
  return net::minecraft::mod::screen_ids::kFatalError;
 }
};
} // namespace net::minecraft::client::gui::screen
