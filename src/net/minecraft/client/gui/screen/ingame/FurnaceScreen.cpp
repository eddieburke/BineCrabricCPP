#include "net/minecraft/client/gui/screen/ingame/FurnaceScreen.hpp"
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/gui/layout/ContainerLayout.hpp"
#include "net/minecraft/entity/player/PlayerInventory.hpp"
#include "net/minecraft/screen/FurnaceScreenHandler.hpp"
#include "net/minecraft/client/gui/Draw2D.hpp"
#include "net/minecraft/client/render/RenderCore.hpp"
#include "net/minecraft/client/render/RenderType.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
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
 {
  const render::RenderPassScope passScope(render::RenderType::guiTextured());
  const float* c = render::core::constColor();
  render::Tessellator& tess = render::Tessellator::INSTANCE;
  tess.startQuads();
  tess.color(c[0], c[1], c[2], c[3]);
  if(furnace_->isBurning()) {
   const int fuelHeight = furnace_->getFuelTimeDelta(12);
   draw::appendAtlasQuad(
       tess, originX + 56, originY + 36 + 12 - fuelHeight, 176, 12 - fuelHeight, 14, fuelHeight + 2, 0.0f);
  }
  const int cookProgress = furnace_->getCookTimeDelta(24);
  draw::appendAtlasQuad(tess, originX + 79, originY + 34, 176, 14, cookProgress + 1, 16, 0.0f);
  tess.draw();
 }
}
} // namespace net::minecraft::client::gui::screen::ingame
