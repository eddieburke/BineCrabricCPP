#pragma once

#include "msauth/MicrosoftAuth.hpp"
#include "net/minecraft/client/gui/screen/Screen.hpp"
#include "net/minecraft/client/gui/widget/ActionButtonWidget.hpp"

#include <atomic>
#include <mutex>
#include <optional>
#include <string>
#include <thread>

namespace net::minecraft::client::gui::auth {

class LoginScreen : public gui::screen::Screen {
public:
    explicit LoginScreen(gui::screen::ScreenFactory returnFactory);
    ~LoginScreen() override;

    void init() override;
    void tick() override;
    void render(int mouseX, int mouseY, float tickDelta) override;

private:
    void tryImportJsonFile();
    void mergePendingImport();
    void showImportError(std::string line2, std::string error);

    gui::screen::ScreenFactory returnFactory_;
    std::string statusLine1_;
    std::string statusLine2_;
    std::string errorMessage_;
    gui::widget::ActionButtonWidget* importButton_ = nullptr;

    std::atomic<bool> importRunning_ {false};
    std::atomic<bool> importFinished_ {false};
    std::mutex importMutex_;
    std::optional<msauth::MicrosoftAccount> pendingImport_;
    std::string pendingImportPath_;
    std::thread importThread_;
    bool firstInit_ = true;
};

} // namespace net::minecraft::client::gui::auth
