#include "msauth/LoginScreen.hpp"

#include "msauth/AccountStorage.hpp"
#include "msauth/FilePicker.hpp"
#include "msauth/MicrosoftAuth.hpp"
#include "msauth/SessionRestore.hpp"
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/gui/layout/ScreenLayout.hpp"
#include "net/minecraft/client/gui/widget/ActionButtonWidget.hpp"
#include "net/minecraft/client/util/MinecraftDirectories.hpp"

namespace msauth {

namespace layout = net::minecraft::client::gui::layout;

LoginScreen::LoginScreen(net::minecraft::client::gui::screen::ScreenFactory returnFactory)
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

    importButton_ = &addCenteredActionButton(layout::menuRowY(height(), 4), "Import account JSON...", [this] {
        tryImportJsonFile();
    });
    importButton_->active = !importRunning_.load();
    addCenteredActionButton(layout::menuRowY(height(), 5), "Cancel", [this] {
        navigateTo(returnFactory_);
    });
}

void LoginScreen::tryImportJsonFile()
{
    if (minecraft() == nullptr || importRunning_.load()) {
        return;
    }

    const std::optional<std::filesystem::path> path = pickJsonFile();
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
        std::optional<MicrosoftAccount> imported = importAccountFromJsonFile(importPath);
        if (imported.has_value() && !imported->valid() && imported->restorable()) {
            const AuthResult restored = restoreFromRefreshToken(*imported);
            if (restored.ok) {
                imported = restored.account;
            } else {
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

    std::optional<MicrosoftAccount> imported;
    std::string pathLabel;
    {
        std::lock_guard<std::mutex> lock(importMutex_);
        imported = std::move(pendingImport_);
        pathLabel = std::move(pendingImportPath_);
    }

    if (!imported.has_value()) {
        statusLine1_ = "Could not import account";
        statusLine2_ = pathLabel;
        errorMessage_ = "Pick a Prism/MultiMC accounts.json or msauth-account.json file.";
        if (importButton_ != nullptr) {
            importButton_->active = true;
        }
        return;
    }

    applyAccount(*imported, minecraft()->session);
    (void)saveAccount(net::minecraft::client::util::MinecraftDirectories::getRunDirectory(), *imported);
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

    drawCenteredTextWithShadow(*textRenderer(), "Microsoft Account", width() / 2, 40, 0xFFFFFF);
    drawCenteredTextWithShadow(*textRenderer(), statusLine1_, width() / 2, 70, 0xFFFFFF);
    if (!statusLine2_.empty()) {
        drawCenteredTextWithShadow(*textRenderer(), statusLine2_, width() / 2, 84, 0xA0A0A0);
    }
    if (!errorMessage_.empty()) {
        drawCenteredTextWithShadow(*textRenderer(), errorMessage_, width() / 2, 106, 0xFF5555);
    }

    Screen::render(mouseX, mouseY, tickDelta);
}

} // namespace msauth
