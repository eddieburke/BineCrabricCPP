#include "net/minecraft/client/gui/screen/ingame/DispenserScreen.hpp"

#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/gui/layout/ContainerLayout.hpp"
#include "net/minecraft/entity/player/PlayerInventory.hpp"
#include "net/minecraft/screen/DispenserScreenHandler.hpp"

namespace net::minecraft::client::gui::screen::ingame {

DispenserScreen::DispenserScreen(
    entity::player::PlayerInventory* playerInventory,
    block::entity::DispenserBlockEntity* dispenser)
    : HandledScreen(nullptr),
      dispenser_(dispenser)
{
    if (playerInventory != nullptr && dispenser_ != nullptr) {
        ownedHandler_ = std::make_unique<::net::minecraft::screen::DispenserScreenHandler>(playerInventory, dispenser_);
        container_ = ownedHandler_.get();
    }
}

void DispenserScreen::drawForeground()
{
    if (textRenderer() == nullptr) {
        return;
    }
    textRenderer_->draw("Dispenser", 60, layout::kContainerTitleY, 0x404040);
    textRenderer_->draw("Inventory", layout::kContainerTitleX, layout::inventoryLabelY(backgroundHeight), 0x404040);
}

void DispenserScreen::drawBackground(float /*tickDelta*/)
{
    if (dispenser_ == nullptr) {
        return;
    }
    drawContainerTexture("/gui/trap.png", 0, 0, backgroundWidth, backgroundHeight);
}

} // namespace net::minecraft::client::gui::screen::ingame
