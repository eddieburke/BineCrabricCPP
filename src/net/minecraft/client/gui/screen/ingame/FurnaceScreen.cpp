#include "net/minecraft/client/gui/screen/ingame/FurnaceScreen.hpp"
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/gui/layout/ContainerLayout.hpp"
#include "net/minecraft/entity/player/PlayerInventory.hpp"
#include "net/minecraft/screen/FurnaceScreenHandler.hpp"
namespace net::minecraft::client::gui::screen::ingame {
FurnaceScreen::FurnaceScreen(entity::player::PlayerInventory* playerInventory,
                             block::entity::FurnaceBlockEntity* furnace)
    : HandledScreen(nullptr), furnace_(furnace) {
 if(playerInventory != nullptr && furnace_ != nullptr) {
  ownedHandler_ = std::make_unique<::net::minecraft::screen::FurnaceScreenHandler>(playerInventory, furnace_);
  container_ = ownedHandler_.get();
 }
}
void FurnaceScreen::drawForeground() {
 if(textRenderer() == nullptr) {
  return;
 }
 textRenderer_->draw("Furnace", 60, layout::kContainerTitleY, 0x404040);
 textRenderer_->draw("Inventory", layout::kContainerTitleX, layout::inventoryLabelY(backgroundHeight), 0x404040);
}
void FurnaceScreen::drawBackground(float /*tickDelta*/) {
 if(furnace_ == nullptr) {
  return;
 }
 drawContainerTexture("/gui/furnace.png", 0, 0, backgroundWidth, backgroundHeight);
 const int originX = containerOriginX();
 const int originY = containerOriginY();
 if(furnace_->isBurning()) {
  const int fuelHeight = furnace_->getFuelTimeDelta(12);
  drawTexture(originX + 56, originY + 36 + 12 - fuelHeight, 176, 12 - fuelHeight, 14, fuelHeight + 2);
 }
 const int cookProgress = furnace_->getCookTimeDelta(24);
 drawTexture(originX + 79, originY + 34, 176, 14, cookProgress + 1, 16);
}
} // namespace net::minecraft::client::gui::screen::ingame
