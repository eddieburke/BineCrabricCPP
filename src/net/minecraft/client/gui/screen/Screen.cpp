#include "net/minecraft/client/gui/screen/Screen.hpp"
#include <typeinfo>
#include <utility>
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/font/TextRenderer.hpp"
#include "net/minecraft/client/gl/GlState.hpp"
#include "net/minecraft/client/gui/Draw2D.hpp"
#include "net/minecraft/client/gui/layout/ScreenLayout.hpp"
#include "net/minecraft/client/gui/screen/TitleScreen.hpp"
#include "net/minecraft/client/gui/screen/option/OptionGui.hpp"
#include "net/minecraft/client/gui/widget/ActionButtonWidget.hpp"
#include "net/minecraft/client/gui/widget/TextFieldWidget.hpp"
#include "net/minecraft/client/input/InputSystem.hpp"
#include "net/minecraft/client/input/Keys.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/client/texture/TextureManager.hpp"
#include "net/minecraft/client/util/UiScale.hpp"
#include "net/minecraft/mod/HookBus.hpp"
#include "net/minecraft/mod/ScreenUi.hpp"
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif
namespace net::minecraft::client::gui::screen {
void Screen::init(client::Minecraft* minecraft, int width, int height) {
  minecraft_ = minecraft;
  textRenderer_ = minecraft != nullptr ? minecraft->textRenderer.get() : nullptr;
  width_ = width;
  height_ = height;
  selectedButton_ = nullptr;
  buttons_.clear();
  init();
  // Generic seam: every screen offers mods a slot once its own widgets exist.
  // Buttons stack from just below the title; screen-specific regions (e.g. the
  // create-world footer) are published separately by those screens.
  int genericButtonY = 6;
  publishScreenUi(mod::screen_regions::kScreen, &genericButtonY);
}
void Screen::publishScreenUi(std::string_view region, int* stackedButtonY) {
  std::string_view screenId = getScreenUiId();
  // Screens that don't declare a friendly id still get one (their typeid name),
  // so mods can address any GUI; friendly ids stay stable for first-party hooks.
  if(screenId.empty()) {
    screenId = typeid(*this).name();
  }
  if(screenId.empty() || region.empty()) {
    return;
  }
  mod::ScreenUiContext context{this, screenId, region, stackedButtonY};
  mod::ScreenUiEvent event{&context};
  mod::hooks().publish(event);
}
void Screen::render(int mouseX, int mouseY, float tickDelta) {
  (void)tickDelta;
  if(minecraft_ == nullptr || textRenderer_ == nullptr) {
    return;
  }
  for(const std::unique_ptr<widget::ButtonWidget>& button : buttons_) {
    if(button == nullptr || !button->visible) {
      continue;
    }
    button->render(*minecraft_, *textRenderer_, mouseX, mouseY);
  }
}
void Screen::tickInput() {
  input::InputSystem::instance().drainScreenEvents(*this);
}
void Screen::onMouseEvent() {
  if(minecraft_ == nullptr) {
    return;
  }
  input::InputSystem& input = input::InputSystem::instance();
  const int dw = minecraft_->displayWidth;
  const int dh = minecraft_->displayHeight;
  if(dw <= 0 || dh <= 0) {
    return;
  }
  const int wheel = input.eventMouseWheel();
  if(wheel != 0) {
    const auto [wheelX, wheelY] =
        util::mapScreenMouse(dw, dh, width_, height_, input.eventMouseX(), input.eventMouseY());
    mouseScrolled(wheelX, wheelY, wheel);
    return;
  }
  const int button = input.eventMouseButton();
  if(button < 0) {
    return;
  }
  const auto [mouseX, mouseY] =
      util::mapScreenMouse(dw, dh, width_, height_, input.eventMouseX(), input.eventMouseY());
  if(input.eventMouseButtonDown()) {
    mouseClicked(mouseX, mouseY, button);
  } else {
    mouseReleased(mouseX, mouseY, button);
  }
}
void Screen::onKeyboardEvent() {
  input::InputSystem& input = input::InputSystem::instance();
  if(!input.eventKeyDown()) {
    return;
  }
  if(input.eventKey() == input::keys::kF11 && minecraft_ != nullptr) {
    minecraft_->toggleFullscreen();
    return;
  }
  keyPressed(input.eventChar(), input.eventKey());
}
bool Screen::pasteChordPressed(int keyCode) noexcept {
#ifdef _WIN32
  return keyCode == input::keys::kV && input::InputSystem::instance().modifiers().ctrl;
#else
  (void)keyCode;
  return false;
#endif
}
bool Screen::closeOnEscape(int keyCode) {
  if(!escapePressed(keyCode) || minecraft_ == nullptr) {
    return false;
  }
  minecraft_->setScreen(nullptr);
  return true;
}
void Screen::keyPressed(char character, int keyCode) {
  (void)character;
  closeOnEscape(keyCode);
}
void Screen::enableTextInput() {
  input::InputSystem::instance().setKeyboardRepeat(true);
}
void Screen::disableTextInput() {
  input::InputSystem::instance().setKeyboardRepeat(false);
}
void Screen::routeKeyToTextFields(char character, int keyCode, std::initializer_list<widget::TextFieldWidget*> fields) {
  for(widget::TextFieldWidget* field : fields) {
    if(field != nullptr && field->focused) {
      field->keyPressed(character, keyCode);
      return;
    }
  }
  for(auto it = fields.end(); it != fields.begin();) {
    --it;
    if(*it != nullptr) {
      (*it)->keyPressed(character, keyCode);
      return;
    }
  }
}
void Screen::clickTextFields(int mouseX,
                             int mouseY,
                             int button,
                             std::initializer_list<widget::TextFieldWidget*> fields) {
  for(widget::TextFieldWidget* field : fields) {
    if(field != nullptr) {
      field->mouseClicked(mouseX, mouseY, button);
    }
  }
}
void Screen::tickTextFields(std::initializer_list<widget::TextFieldWidget*> fields) {
  for(widget::TextFieldWidget* field : fields) {
    if(field != nullptr) {
      field->tick();
    }
  }
}
void Screen::handleFormKeyPress(char character,
                                int keyCode,
                                std::initializer_list<widget::TextFieldWidget*> fields,
                                const std::function<void()>& onSubmit) {
  routeKeyToTextFields(character, keyCode, fields);
  if(submitPressed(keyCode, character)) {
    onSubmit();
    return;
  }
  closeOnEscape(keyCode);
}
void Screen::mouseClicked(int mouseX, int mouseY, int button) {
  if(button != 0) {
    return;
  }
  selectedButton_ = nullptr;
  for(const std::unique_ptr<widget::ButtonWidget>& widget : buttons_) {
    if(widget == nullptr || !widget->isMouseOver(mouseX, mouseY)) {
      continue;
    }
    selectedButton_ = widget.get();
    minecraft_->audio.play("random.click", 1.0f, 1.0f);
    dispatchButtonPress(*widget);
    break;
  }
}
widget::ActionButtonWidget& Screen::addActionButton(int x, int y, std::string text, std::function<void()> onClick) {
  return addActionButton(
      x, y, layout::kDefaultButtonWidth, layout::kDefaultButtonHeight, std::move(text), std::move(onClick));
}
widget::ActionButtonWidget& Screen::addActionButton(
    int x, int y, int width, int height, std::string text, std::function<void()> onClick) {
  return addActionButton(widget::ActionButtonWidget::kNoId, x, y, width, height, std::move(text), std::move(onClick));
}
widget::ActionButtonWidget& Screen::addActionButton(
    int id, int x, int y, int width, int height, std::string text, std::function<void()> onClick) {
  auto button =
      std::make_unique<widget::ActionButtonWidget>(id, x, y, width, height, std::move(text), std::move(onClick));
  widget::ActionButtonWidget& ref = *button;
  buttons_.push_back(std::move(button));
  return ref;
}
widget::ActionButtonWidget& Screen::addCenteredActionButton(int y, std::string text, std::function<void()> onClick) {
  return addCenteredActionButton(
      y, layout::kDefaultButtonWidth, layout::kDefaultButtonHeight, std::move(text), std::move(onClick));
}
widget::ActionButtonWidget& Screen::addCenteredActionButton(
    int y, int width, int height, std::string text, std::function<void()> onClick) {
  return addActionButton(layout::centerBtnX(width_), y, width, height, std::move(text), std::move(onClick));
}
void Screen::dispatchButtonPress(widget::ButtonWidget& button) {
  if(auto* actionButton = dynamic_cast<widget::ActionButtonWidget*>(&button)) {
    if(actionButton->onClick) {
      actionButton->onClick();
      return;
    }
  }
  option::handleOptionButtonClick(*this, button);
}
void Screen::closeScreen() {
  if(minecraft_ != nullptr) {
    minecraft_->setScreen(nullptr);
  }
}
void Screen::navigateTo(ScreenFactory factory) {
  if(minecraft_ != nullptr && factory) {
    minecraft_->setScreen(factory());
  }
}
void Screen::navigateTo(std::unique_ptr<Screen> screen) {
  if(minecraft_ != nullptr) {
    minecraft_->setScreen(std::move(screen));
  }
}
void Screen::quitToTitle() {
  if(minecraft_ != nullptr) {
    minecraft_->setWorld(nullptr);
    minecraft_->setScreen(std::make_unique<TitleScreen>());
  }
}
void Screen::quitGame() {
  if(minecraft_ != nullptr) {
    minecraft_->scheduleStop();
  }
}
void Screen::mouseReleased(int mouseX, int mouseY, int button) {
  if(selectedButton_ != nullptr && button == 0) {
    selectedButton_->mouseReleased(mouseX, mouseY);
    selectedButton_ = nullptr;
  }
}
void Screen::renderBackground() {
  renderBackground(0);
}
void Screen::renderBackground(int vOffset) {
  if(minecraft_ != nullptr && minecraft_->world != nullptr) {
    fillGradient(0, 0, width_, height_, 0xC0101010U, 0xD0101010U);
  } else {
    renderBackgroundTexture(vOffset);
  }
}
void Screen::renderBackgroundTexture(int vOffset) {
  if(minecraft_ == nullptr) {
    fillGradient(0, 0, width_, height_, 0xFF101020U, 0xFF202040U);
    return;
  }
  render::Tessellator& tessellator = render::INSTANCE;
  const int textureId = minecraft_->textureManager.getTextureId("/gui/background.png");
  gl::pass::bindAtlas2D(textureId);
  const gl::preset::ScreenFogOff fogCaps;
  draw::tiledPanel(tessellator, 0, 0, width_, height_, static_cast<float>(vOffset), 0x404040);
}
void Screen::confirmed(bool confirmed, int id) {
  (void)confirmed;
  (void)id;
}
void Screen::handleTab() {
}
std::string Screen::getClipboard() {
#ifdef _WIN32
  if(!OpenClipboard(nullptr)) {
    return {};
  }
  HANDLE data = GetClipboardData(CF_UNICODETEXT);
  if(data == nullptr) {
    CloseClipboard();
    return {};
  }
  const auto* wide = static_cast<const wchar_t*>(GlobalLock(data));
  if(wide == nullptr) {
    CloseClipboard();
    return {};
  }
  std::string result;
  for(const wchar_t* p = wide; *p != L'\0'; ++p) {
    const wchar_t ch = *p;
    if(ch <= 0x7F) {
      result.push_back(static_cast<char>(ch));
    }
  }
  GlobalUnlock(data);
  CloseClipboard();
  return result;
#else
  return {};
#endif
}
} // namespace net::minecraft::client::gui::screen
