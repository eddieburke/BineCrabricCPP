#include "net/minecraft/client/gui/screen/ChatScreen.hpp"
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/gui/Draw2D.hpp"
#include "net/minecraft/client/gui/hud/InGameHud.hpp"
#include "net/minecraft/client/platform/Browser.hpp"
#include "net/minecraft/client/render/RenderType.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/entity/player/ClientPlayerEntity.hpp"
#include "net/minecraft/mod/runtime/LuaDirectHooks.hpp"
#include "net/minecraft/util/CharacterUtils.hpp"
namespace net::minecraft::client::gui::screen {
void ChatScreen::init() {
 text_.clear();
 historyOffset_ = -1;
 pendingText_.clear();
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
 minecraft()->inGameHud.addSentChatMessage(event.message);
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
 if(arrowUpPressed(keyCode)) {
  recallHistory(1);
  return;
 }
 if(arrowDownPressed(keyCode)) {
  recallHistory(-1);
  return;
 }
 if(keyCode == input::keys::kPageUp && minecraft() != nullptr) {
  minecraft()->inGameHud.scrollChat(19);
  return;
 }
 if(keyCode == input::keys::kPageDown && minecraft() != nullptr) {
  minecraft()->inGameHud.scrollChat(-19);
  return;
 }
 if(copyChordPressed(keyCode)) {
  setClipboard(text_);
  return;
 }
 if(character == '\x16' || pasteChordPressed(keyCode)) {
  const std::string clipboard = getClipboard();
  const std::string& valid = CharacterUtils::validCharacters();
  for(char value : clipboard) {
   if(text_.length() >= 100) {
    break;
   }
   if(valid.find(value) != std::string::npos) {
    text_.push_back(value);
   }
  }
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
 if(minecraft() == nullptr) {
  Screen::mouseClicked(mouseX, mouseY, button);
  return;
 }
 if(button == 1) {
  const std::string line = minecraft()->inGameHud.chatLineAt(mouseX, mouseY, height());
  if(!line.empty()) {
   setClipboard(line);
   return;
  }
 }
 if(button != 0) {
  Screen::mouseClicked(mouseX, mouseY, button);
  return;
 }
 const std::string link = minecraft()->inGameHud.chatLinkAt(mouseX, mouseY, height());
 if(!link.empty()) {
  (void)platform::openUrlInBrowser(link);
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
void ChatScreen::mouseScrolled(int mouseX, int mouseY, int delta) {
 (void)mouseX;
 (void)mouseY;
 if(minecraft() != nullptr && delta != 0) {
  minecraft()->inGameHud.scrollChat(delta < 0 ? 1 : -1);
 }
}
void ChatScreen::recallHistory(int direction) {
 if(minecraft() == nullptr || direction == 0) {
  return;
 }
 const auto& history = minecraft()->inGameHud.sentChatMessages();
 if(history.empty()) {
  return;
 }
 if(historyOffset_ < 0 && direction > 0) {
  pendingText_ = text_;
 }
 historyOffset_ = std::clamp(historyOffset_ + direction, -1, static_cast<int>(history.size()) - 1);
 if(historyOffset_ < 0) {
  text_ = pendingText_;
 } else {
  text_ = history[history.size() - 1 - static_cast<std::size_t>(historyOffset_)];
 }
}
} // namespace net::minecraft::client::gui::screen
