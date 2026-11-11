#include "net/minecraft/client/gui/screen/ingame/SignEditScreen.hpp"
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/block/entity/SignBlockEntity.hpp"
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/font/TextRenderer.hpp"
#include "net/minecraft/client/gui/layout/ScreenLayout.hpp"
#include "net/minecraft/client/multiplayer/MultiplayerClientPlayerEntity.hpp"
#include "net/minecraft/client/render/RenderCore.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/client/render/block/entity/BlockEntityRenderDispatcher.hpp"
#include "net/minecraft/client/resource/language/I18n.hpp"
#include "net/minecraft/network/packet/InventoryPackets.hpp"
#include "net/minecraft/util/CharacterUtils.hpp"
#include "net/minecraft/util/math/MatrixStack.hpp"
#include "net/minecraft/world/World.hpp"

namespace net::minecraft::client::gui::screen::ingame {
namespace core = net::minecraft::client::render::core;

SignEditScreen::SignEditScreen(block::entity::SignBlockEntity* sign) : sign_(sign) {
 if(sign_ != nullptr) {
  text = sign_->texts;
  sign_->currentRow = currentLine;
 }
}

void SignEditScreen::init() {
 buttons_.clear();
 enableTextInput();
 addCenteredActionButton(height() / 4 + 120, resource::language::I18n::getTranslation("gui.done"), [this] {
  closeScreen();
 });
}

void SignEditScreen::removed() {
 disableTextInput();
 if(sign_ != nullptr) {
  sign_->texts = text;
  sign_->currentRow = -1;
  sign_->setEditable(false);
  sign_->markDirty();
  if(sign_->world != nullptr) {
   sign_->world->blockUpdateEvent(sign_->x, sign_->y, sign_->z);
  }
  if(minecraft() != nullptr) {
   if(auto* mpPlayer = dynamic_cast<multiplayer::MultiplayerClientPlayerEntity*>(minecraft()->player)) {
    if(mpPlayer->networkHandler != nullptr) {
     UpdateSignPacket packet;
     packet.x = sign_->x;
     packet.y = sign_->y;
     packet.z = sign_->z;
     packet.text = text;
     mpPlayer->networkHandler->sendPacket(packet);
    }
   }
  }
 }
}

void SignEditScreen::tick() {
 ++ticks_;
}

// see mcp/src/net/minecraft/src/GuiEditSign.java:81
void SignEditScreen::render(int mouseX, int mouseY, float tickDelta) {
 renderBackground();
 if(textRenderer() != nullptr) {
  textRenderer()->drawCenteredWithShadow("Edit sign message:", width() / 2, 40, 0xFFFFFF);
 }
 if(sign_ != nullptr) {
  const core::DepthScope depthScope(true, true);
  const core::ScopedDrawCameraState cameraGuard;
  net::minecraft::util::math::MatrixStack matrices;
  matrices.load(core::drawPose());
  matrices.translate(static_cast<float>(width() / 2), 0.0f, 50.0f);
  const float scale = 93.75f;
  matrices.scale(-scale, -scale, -scale);
  matrices.rotate(180.0f, 0.0f, 1.0f, 0.0f);
  block::Block* block = sign_->getBlock();
  if(block == block::Block::SIGN) {
   const float yaw = static_cast<float>(sign_->getPushedBlockData() * 360) / 16.0f;
   matrices.rotate(yaw, 0.0f, 1.0f, 0.0f);
   matrices.translate(0.0f, -1.0625f, 0.0f);
  } else {
   const int meta = sign_->getPushedBlockData();
   float yaw = 0.0f;
   if(meta == 2) {
    yaw = 180.0f;
   } else if(meta == 4) {
    yaw = 90.0f;
   } else if(meta == 5) {
    yaw = -90.0f;
   }
   matrices.rotate(yaw, 0.0f, 1.0f, 0.0f);
   matrices.translate(0.0f, -1.0625f, 0.0f);
  }
  if((ticks_ / 6) % 2 == 0) {
   sign_->currentRow = currentLine;
  }
  core::setDrawPose(matrices.top());
  core::setConstColor(1.0f, 1.0f, 1.0f, 1.0f);
  render::Tessellator::INSTANCE.light(15, 15);
  auto& dispatcher = render::block::entity::BlockEntityRenderDispatcher::instance();
  if(dispatcher.textureManager == nullptr && minecraft() != nullptr) {
   dispatcher.textureManager = &minecraft()->textureManager;
  }
  if(dispatcher.textRenderer == nullptr && textRenderer() != nullptr) {
   dispatcher.textRenderer = textRenderer();
  }
  if(dispatcher.world == nullptr && sign_->world != nullptr) {
   dispatcher.setWorld(sign_->world);
  }
  dispatcher.render(*sign_, -0.5, -0.75, -0.5, 0.0f);
  sign_->currentRow = -1;
 }
 Screen::render(mouseX, mouseY, tickDelta);
}

void SignEditScreen::keyPressed(char character, int keyCode) {
 if(closeOnEscape(keyCode)) {
  return;
 }
 if(arrowUpPressed(keyCode)) {
  currentLine = (currentLine + 3) & 3;
  if(sign_ != nullptr) {
   sign_->currentRow = currentLine;
  }
  return;
 }
 if(arrowDownPressed(keyCode) || submitPressed(keyCode, character)) {
  currentLine = (currentLine + 1) & 3;
  if(sign_ != nullptr) {
   sign_->currentRow = currentLine;
  }
  return;
 }
 if(backspacePressed(keyCode)) {
  if(!text[static_cast<std::size_t>(currentLine)].empty()) {
   text[static_cast<std::size_t>(currentLine)].pop_back();
   if(sign_ != nullptr) {
    sign_->texts[static_cast<std::size_t>(currentLine)] = text[static_cast<std::size_t>(currentLine)];
   }
  }
  return;
 }
 constexpr int kMaxSignLineLength = 15;
 if(character == '\x16' || pasteChordPressed(keyCode)) {
  const std::string clipboard = getClipboard();
  const std::string& valid = CharacterUtils::validCharacters();
  for(char value : clipboard) {
   if(text[static_cast<std::size_t>(currentLine)].size() >= kMaxSignLineLength) {
    break;
   }
   if(value != 0 && (valid.empty() || valid.find(value) != std::string::npos)) {
    text[static_cast<std::size_t>(currentLine)].push_back(value);
   }
  }
  if(sign_ != nullptr) {
   sign_->texts[static_cast<std::size_t>(currentLine)] = text[static_cast<std::size_t>(currentLine)];
  }
  return;
 }
 if(character == '\0') {
  return;
 }
 const std::string& valid = CharacterUtils::validCharacters();
 if((valid.empty() || valid.find(character) != std::string::npos) &&
    text[static_cast<std::size_t>(currentLine)].size() < kMaxSignLineLength) {
  text[static_cast<std::size_t>(currentLine)].push_back(character);
  if(sign_ != nullptr) {
   sign_->texts[static_cast<std::size_t>(currentLine)] = text[static_cast<std::size_t>(currentLine)];
  }
 }
}

} // namespace net::minecraft::client::gui::screen::ingame
