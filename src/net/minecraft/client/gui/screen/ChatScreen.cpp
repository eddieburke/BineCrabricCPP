#include "net/minecraft/client/gui/screen/ChatScreen.hpp"
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/gui/Draw2D.hpp"
#include "net/minecraft/client/gui/hud/InGameHud.hpp"
#include "net/minecraft/client/render/RenderType.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/entity/player/ClientPlayerEntity.hpp"
#include "net/minecraft/mod/runtime/LuaDirectHooks.hpp"
#include "net/minecraft/util/CharacterUtils.hpp"
namespace net::minecraft::client::gui::screen {
void ChatScreen::init() {
 text_.clear();
 enableTextInput();
}
void ChatScreen::removed() {
 disableTextInput();
}
void ChatScreen::tick() {
 ++focusedTicks_;
}
std::string ChatScreen::trimmedText() const {
 const std::size_t start = text_.find_first_not_of(" \t");
 if(start == std::string::npos) {
  return {};
 }
 const std::size_t end = text_.find_last_not_of(" \t");
 return text_.substr(start, end - start + 1);
}
void ChatScreen::sendCurrentText() {
 if(minecraft() == nullptr || minecraft()->player == nullptr) {
  return;
 }
 net::minecraft::mod::ChatEvent event;
 event.world = minecraft()->world;
 event.message = trimmedText();
 if(event.message.empty()) {
  return;
 }
 net::minecraft::mod::runtime::luaHookChatSend(event);
 if(event.canceled || event.message.empty()) {
  return;
 }
 minecraft()->player->sendChatMessage(event.message);
}
void ChatScreen::render(int mouseX, int mouseY, float tickDelta) {
 (void)mouseX;
 (void)mouseY;
 (void)tickDelta;
 {
  const render::RenderPassScope passScope(render::RenderType::gui());
  draw::coloredQuad(render::Tessellator::INSTANCE, width_ - 2, height_ - 2, 2, height_ - 14, 0, 0x80);
 }
 if(textRenderer() != nullptr) {
  const std::string cursor = (focusedTicks_ / 6 % 2 == 0) ? "_" : "";
  textRenderer()->drawWithShadow("> " + text_ + cursor, 4, height_ - 12, 0xE0E0E0);
 }
 Screen::render(mouseX, mouseY, tickDelta);
}
void ChatScreen::keyPressed(char character, int keyCode) {
 if(closeOnEscape(keyCode)) {
  return;
 }
 if(submitPressed(keyCode, character)) {
  if(minecraft() != nullptr) {
   sendCurrentText();
   minecraft()->setScreen(nullptr);
  }
  return;
 }
 if(backspacePressed(keyCode) && !text_.empty()) {
  text_.pop_back();
  return;
 }
 if(character == '\0') {
  return;
 }
 const std::string& valid = CharacterUtils::validCharacters();
 if(valid.find(character) != std::string::npos && text_.length() < 100) {
  text_.push_back(character);
 }
}
void ChatScreen::mouseClicked(int mouseX, int mouseY, int button) {
 if(button != 0 || minecraft() == nullptr) {
  Screen::mouseClicked(mouseX, mouseY, button);
  return;
 }
 if(!minecraft()->inGameHud.selectedName.empty()) {
  if(!text_.empty() && text_.back() != ' ') {
   text_.push_back(' ');
  }
  text_ += minecraft()->inGameHud.selectedName;
  if(text_.length() > 100) {
   text_.resize(100);
  }
  return;
 }
 Screen::mouseClicked(mouseX, mouseY, button);
}
} // namespace net::minecraft::client::gui::screen
