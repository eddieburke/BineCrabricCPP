#include "net/minecraft/client/gui/screen/ingame/DoubleChestScreen.hpp"
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/gui/layout/ContainerLayout.hpp"
#include "net/minecraft/entity/player/PlayerInventory.hpp"
#include "net/minecraft/screen/GenericContainerScreenHandler.hpp"
namespace net::minecraft::client::gui::screen::ingame {
DoubleChestScreen::DoubleChestScreen(entity::player::PlayerInventory* playerInventory, Inventory* inventory)
    : HandledScreen(nullptr), playerInventory_(playerInventory), inventory_(inventory) {
  passEvents = false;
  if(playerInventory_ != nullptr && inventory_ != nullptr) {
    rows_ = static_cast<int>(inventory_->size() / 9);
    backgroundHeight = 114 + rows_ * 18;
    ownedHandler_ =
        std::make_unique<::net::minecraft::screen::GenericContainerScreenHandler>(playerInventory_, inventory_);
    container_ = ownedHandler_.get();
  }
}
void DoubleChestScreen::drawForeground() {
  if(textRenderer() == nullptr || inventory_ == nullptr || playerInventory_ == nullptr) {
    return;
  }
  textRenderer_->draw(inventory_->getName(), layout::kContainerTitleX, layout::kContainerTitleY, 0x404040);
  textRenderer_->draw(playerInventory_->getName(), layout::kContainerTitleX,
                      layout::inventoryLabelY(backgroundHeight), 0x404040);
}
void DoubleChestScreen::drawBackground(float /*tickDelta*/) {
  drawContainerTextureSplit("/gui/container.png", rows_ * layout::kSlotStep + 17, 126, 96);
}
} // namespace net::minecraft::client::gui::screen::ingame
