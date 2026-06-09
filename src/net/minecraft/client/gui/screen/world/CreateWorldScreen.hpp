#pragma once

// Faithful port of gui.screen.world.CreateWorldScreen (beta 1.7.3 MCP).

#include "net/minecraft/client/gui/screen/Screen.hpp"
#include "net/minecraft/client/gui/widget/TextFieldWidget.hpp"

#include <memory>
#include <string>

namespace net::minecraft {
class WorldStorageSource;
}

namespace net::minecraft::client::gui::screen::world {

class CreateWorldScreen : public screen::Screen {
public:
    explicit CreateWorldScreen(screen::ScreenFactory parentFactory = {},
        std::string initialWorldName = {},
        std::string initialSeedText = {});
    ~CreateWorldScreen() override = default;

    void init() override;
    void tick() override;
    void removed() override;
    void render(int mouseX, int mouseY, float tickDelta) override;
    void keyPressed(char character, int keyCode) override;
    void mouseClicked(int mouseX, int mouseY, int button) override;
    void handleTab() override;

    [[nodiscard]] static std::string getWorldSaveName(WorldStorageSource* storageSource, std::string worldName);

private:
    void getSaveDirectoryNames();
    void createWorld();
    void updateCreateButtonState();

    screen::ScreenFactory parentFactory_;
    std::string initialWorldName_;
    std::string initialSeedText_;
    std::unique_ptr<widget::TextFieldWidget> worldNameField_;
    std::unique_ptr<widget::TextFieldWidget> seedField_;
    std::string worldSaveName_;
    bool creatingLevel_ = false;
    widget::ActionButtonWidget* createButton_ = nullptr;
};

} // namespace net::minecraft::client::gui::screen::world
