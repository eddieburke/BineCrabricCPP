#pragma once
#include <memory>

#include "net/minecraft/block/entity/FurnaceBlockEntity.hpp"
#include "net/minecraft/client/gui/screen/ingame/HandledScreen.hpp"
#include "net/minecraft/screen/ScreenHandler.hpp"

namespace net::minecraft::entity::player {
class PlayerInventory;
}

namespace net::minecraft::client::gui::screen::ingame {
class FurnaceScreen : public HandledScreen {
   public:
    FurnaceScreen(entity::player::PlayerInventory* playerInventory, block::entity::FurnaceBlockEntity* furnace);

   protected:
    void drawForeground() override;
    void drawBackground(float tickDelta) override;

    [[nodiscard]] std::string_view getScreenUiId() const override {
        return net::minecraft::mod::screen_ids::kFurnace;
    }

   private:
    block::entity::FurnaceBlockEntity* furnace_ = nullptr;
    std::unique_ptr<::net::minecraft::screen::ScreenHandler> ownedHandler_;
};
}  // namespace net::minecraft::client::gui::screen::ingame
