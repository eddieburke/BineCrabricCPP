#pragma once

#include "net/minecraft/client/gui/screen/Screen.hpp"
#include "net/minecraft/client/gui/widget/EntryListWidget.hpp"

#include <filesystem>
#include <memory>

namespace net::minecraft::client::gui::screen::pack {

class PackScreen : public screen::Screen {
public:
    explicit PackScreen(screen::ScreenFactory parentFactory = {});
    ~PackScreen() override;

    void init() override;
    void render(int mouseX, int mouseY, float tickDelta) override;
    void tick() override;

private:
    class PackListWidget;
    void openTexturePacksFolder();

    screen::ScreenFactory parentFactory_;
    std::unique_ptr<PackListWidget> packList_;
    int reloadCooldown_ = -1;
    std::filesystem::path texturePacksDir_;
};

} // namespace net::minecraft::client::gui::screen::pack
