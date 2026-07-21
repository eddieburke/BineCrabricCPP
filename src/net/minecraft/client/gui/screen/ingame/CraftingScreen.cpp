#include "net/minecraft/client/gui/screen/ingame/CraftingScreen.hpp"
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/gui/layout/ContainerLayout.hpp"
#include "net/minecraft/entity/player/ClientPlayerEntity.hpp"
#include "net/minecraft/entity/player/PlayerInventory.hpp"
#include "net/minecraft/screen/CraftingScreenHandler.hpp"
namespace net::minecraft::client::gui::screen::ingame {
CraftingScreen::CraftingScreen(entity::player::PlayerInventory* playerInventory, int x, int y, int z)
    : HandledScreen(nullptr) {
 if(playerInventory != nullptr) {
  ownedHandler_ = std::make_unique<::net::minecraft::screen::CraftingScreenHandler>(playerInventory, x, y, z);
  container_ = ownedHandler_.get();
 }
}
void CraftingScreen::removed() {
 HandledScreen::removed();
 if(minecraft_ != nullptr && minecraft_->player != nullptr && container_ != nullptr) {
  container_->onClosed(minecraft_->player);
 }
}
void CraftingScreen::drawForeground() {
 if(textRenderer() == nullptr) {
  return;
 }
 textRenderer_->draw("Crafting", 28, layout::kContainerTitleY, 0x404040);
 textRenderer_->draw("Inventory", layout::kContainerTitleX, layout::inventoryLabelY(backgroundHeight), 0x404040);
}
void CraftingScreen::drawBackground(float /*tickDelta*/) {
 drawContainerTexture("/gui/crafting.png", 0, 0, backgroundWidth, backgroundHeight);
}
} // namespace net::minecraft::client::gui::screen::ingame
