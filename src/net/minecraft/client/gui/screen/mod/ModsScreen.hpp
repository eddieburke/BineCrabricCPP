#pragma once
#include <filesystem>
#include <memory>
#include <vector>

#include "net/minecraft/client/gui/screen/Screen.hpp"
#include "net/minecraft/client/gui/widget/EntryListWidget.hpp"
#include "net/minecraft/mod/runtime/ModHost.hpp"

namespace net::minecraft::client::gui::screen::mod {
class ModsScreen : public screen::Screen {
   public:
    explicit ModsScreen(screen::ScreenFactory parentFactory = {});
    ~ModsScreen() override;
    void init() override;
    void render(int mouseX, int mouseY, float tickDelta) override;
    void tick() override;

    [[nodiscard]] std::string_view getScreenUiId() const override {
        return net::minecraft::mod::screen_ids::kMods;
    }

   private:
    class ModListWidget;
    void refreshMods();
    void toggleSelected();
    void updateToggleButton();
    void openModsFolder();
    void openTexturePacks();
    screen::ScreenFactory parentFactory_;
    std::unique_ptr<ModListWidget> modList_;
    std::vector<net::minecraft::mod::runtime::ModPackage> mods_;
    std::filesystem::path modsDir_;
    widget::ActionButtonWidget* toggleButton_ = nullptr;
    int selectedIndex_ = -1;
    std::string footerText_;
};
}  // namespace net::minecraft::client::gui::screen::mod
