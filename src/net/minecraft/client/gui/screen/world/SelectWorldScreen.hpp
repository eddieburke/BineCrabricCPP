#pragma once

// Faithful port of gui.screen.world.SelectWorldScreen (beta 1.7.3 MCP).

#include "net/minecraft/client/gui/screen/Screen.hpp"
#include "net/minecraft/client/gui/widget/EntryListWidget.hpp"
#include "net/minecraft/world/storage/WorldSaveInfo.hpp"

#include <memory>
#include <string>
#include <vector>

namespace net::minecraft::client::gui::screen::world {

class SelectWorldScreen : public screen::Screen {
public:
    explicit SelectWorldScreen(screen::ScreenFactory parentFactory = {});
    ~SelectWorldScreen() override;

    void init() override;
    void render(int mouseX, int mouseY, float tickDelta) override;
    void confirmed(bool confirmed, int id) override;

protected:
    void buttonClicked(widget::ButtonWidget& button) override;

private:
    class WorldListWidget;

    void getSaves();
    void addButtons();
    void confirmDeleteWorld();
    [[nodiscard]] std::string getSaveFileName(int index) const;
    [[nodiscard]] std::string getWorldName(int index) const;
    void selectWorld(int id);

    screen::ScreenFactory parentFactory_;
    std::string title_;
    std::string worldText_;
    std::string conversionText_;
    bool selected_ = false;
    int selectedWorldId_ = -1;
    std::vector<WorldSaveInfo> saves_;
    std::unique_ptr<WorldListWidget> worldList_;
    widget::ActionButtonWidget* playSelectedWorldButton_ = nullptr;
    widget::ActionButtonWidget* renameWorldButton_ = nullptr;
    widget::ActionButtonWidget* deleteWorldButton_ = nullptr;
};

} // namespace net::minecraft::client::gui::screen::world
