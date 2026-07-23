#include "net/minecraft/client/gui/screen/SleepingChatScreen.hpp"
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/gui/layout/ScreenLayout.hpp"
#include "net/minecraft/client/input/InputSystem.hpp"
#include "net/minecraft/client/input/Keys.hpp"
#include "net/minecraft/client/multiplayer/MultiplayerClientPlayerEntity.hpp"
#include "net/minecraft/client/resource/language/I18n.hpp"
#include "net/minecraft/network/packet/PlayerPackets.hpp"
namespace net::minecraft::client::gui::screen {
void SleepingChatScreen::init() {
 ChatScreen::init();
 addCenteredActionButton(height() - 40,
                         resource::language::I18n::getTranslation("multiplayer.stopSleeping"),
                         [this] { stopSleeping(); });
}
void SleepingChatScreen::removed() {
 ChatScreen::removed();
}
void SleepingChatScreen::keyPressed(char character, int keyCode) {
 if(escapePressed(keyCode)) {
  stopSleeping();
  return;
 }
 if(submitPressed(keyCode, character)) {
  sendCurrentText();
  text_.clear();
  return;
 }
 ChatScreen::keyPressed(character, keyCode);
}
void SleepingChatScreen::stopSleeping() {
 if(minecraft() == nullptr || minecraft()->player == nullptr) {
  return;
 }
 if(auto* mpPlayer = dynamic_cast<multiplayer::MultiplayerClientPlayerEntity*>(minecraft()->player)) {
  if(mpPlayer->networkHandler != nullptr) {
   constexpr int kStopSleeping = 3;
   ClientCommandC2SPacket packet;
   packet.entityId = mpPlayer->id;
   packet.mode = kStopSleeping;
   mpPlayer->networkHandler->sendPacket(packet);
  }
 }
}
} // namespace net::minecraft::client::gui::screen
