#include "net/minecraft/client/gui/auth/LoginScreen.hpp"

#include "msauth/AccountStorage.hpp"
#include "msauth/FilePicker.hpp"
#include "msauth/MicrosoftAuth.hpp"
#include "msauth/SessionRestore.hpp"
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/gui/widget/ActionButtonWidget.hpp"
#include "net/minecraft/client/util/MinecraftDirectories.hpp"

namespace net::minecraft::client::gui::auth {

LoginScreen::LoginScreen(gui::screen::ScreenFactory returnFactory)
    : returnFactory_(std::move(returnFactory))
{
}

LoginScreen::~LoginScreen()
{
    if (importThread_.joinable()) {
        importThread_.join();
    }
}

void LoginScreen::init()
{
    buttons_.clear();
    importButton_ = nullptr;
    const bool resizing = !firstInit_;
    firstInit_ = false;

    if (!resizing) {
        statusLine1_ = "Import an account JSON file to sign in.";
        statusLine2_ = "Prism/MultiMC accounts.json or msauth-account.json";
        errorMessage_.clear();
    }

    importButton_ = &addCenteredActionButton(height() / 4 + 78, "Import account JSON...", [this] {
        tryImportJsonFile();
    });
    importButton_->active = !importRunning_.load();
    addCenteredActionButton(height() / 4 + 102, "Cancel", [this] {
        navigateTo(returnFactory_);
    });
}

void LoginScreen::showImportError(std::string line2, std::string error)
{
    statusLine1_ = "Could not import account";
    statusLine2_ = std::move(line2);
    errorMessage_ = std::move(error);
    if (importButton_ != nullptr) {
        importButton_->active = true;
    }
}

void LoginScreen::tryImportJsonFile()
{
    if (minecraft() == nullptr || importRunning_.load()) {
        return;
    }

    const std::optional<std::filesystem::path> path = msauth::pickJsonFile();
    if (!path.has_value()) {
        return;
    }

    if (importThread_.joinable()) {
        importThread_.join();
    }

    importRunning_ = true;
    importFinished_ = false;
    errorMessage_.clear();
    statusLine1_ = "Reading account JSON...";
    statusLine2_ = path->filename().string();
    if (importButton_ != nullptr) {
        importButton_->active = false;
    }

    const std::filesystem::path importPath = *path;
    importThread_ = std::thread([this, importPath]() {
        std::optional<msauth::MicrosoftAccount> imported = msauth::importAccountFromJsonFile(importPath);
        if (imported.has_value() && imported->restorable()) {
            const msauth::AuthResult restored = msauth::restoreFromRefreshToken(*imported);
            if (restored.ok) {
                imported = restored.account;
            } else if (!imported->valid()) {
                imported.reset();
            }
        }
        {
            std::lock_guard<std::mutex> lock(importMutex_);
            pendingImport_ = std::move(imported);
            pendingImportPath_ = importPath.filename().string();
        }
        importFinished_ = true;
    });
}

void LoginScreen::mergePendingImport()
{
    if (!importFinished_.exchange(false)) {
        return;
    }
    if (importThread_.joinable()) {
        importThread_.join();
    }
    importRunning_ = false;

    std::optional<msauth::MicrosoftAccount> imported;
    std::string pathLabel;
    {
        std::lock_guard<std::mutex> lock(importMutex_);
        imported = std::move(pendingImport_);
        pathLabel = std::move(pendingImportPath_);
    }

    if (!imported.has_value()) {
        showImportError(pathLabel, "Pick a Prism/MultiMC accounts.json or msauth-account.json file.");
        return;
    }

    msauth::applyAccount(*imported, minecraft()->session);
    if (!msauth::saveAccount(util::MinecraftDirectories::getRunDirectory(), *imported)) {
        showImportError(
            imported->profileName,
            "Signed in, but no refresh token to save. Re-export from Prism while signed in.");
        return;
    }
    navigateTo(returnFactory_);
}

void LoginScreen::tick()
{
    mergePendingImport();
}

void LoginScreen::render(int mouseX, int mouseY, float tickDelta)
{
    renderBackground();
    if (textRenderer() == nullptr) {
        return;
    }

    const int centerX = width() / 2;
    const int top = height() / 4;
    drawCenteredTextWithShadow(*textRenderer(), "Microsoft Account", centerX, top - 30, 0xFFFFFF);
    drawCenteredTextWithShadow(*textRenderer(), statusLine1_, centerX, top + 8, 0xFFFFFF);
    if (!statusLine2_.empty()) {
        drawCenteredTextWithShadow(*textRenderer(), statusLine2_, centerX, top + 22, 0xA0A0A0);
    }
    if (!errorMessage_.empty()) {
        drawCenteredTextWithShadow(*textRenderer(), errorMessage_, centerX, top + 48, 0xFF5555);
    }

    Screen::render(mouseX, mouseY, tickDelta);
}

} // namespace net::minecraft::client::gui::auth
