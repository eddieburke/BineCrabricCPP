#include "net/minecraft/client/gui/auth/AccountUiState.hpp"

#include "msauth/AccountStorage.hpp"
#include "msauth/SessionRestore.hpp"
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/gui/auth/LoginScreen.hpp"
#include "net/minecraft/client/util/MinecraftDirectories.hpp"

namespace net::minecraft::client::gui::auth {

AccountUiSnapshot pollAccountUi(const Minecraft& client)
{
    AccountUiSnapshot snapshot;
    const std::filesystem::path runDirectory = util::MinecraftDirectories::getRunDirectory();

    snapshot.multiplayerReady = msauth::isAuthenticated(client.session);
    snapshot.showSignOutButton =
        snapshot.multiplayerReady || msauth::hasRestorableSavedAccount(runDirectory);
    snapshot.buttonLabel = snapshot.showSignOutButton ? "Sign out" : "Sign in";

    if (snapshot.multiplayerReady) {
        snapshot.statusLine = "Signed in as " + client.session.username;
        return snapshot;
    }

    if (msauth::isSavedAccountRestorePending()) {
        if (const std::optional<std::string> profileName = msauth::savedAccountProfileName(runDirectory)) {
            snapshot.statusLine = "Restoring " + *profileName + "...";
        } else {
            snapshot.statusLine = "Restoring account...";
        }
    }

    return snapshot;
}

void signOutAccount(Minecraft& client)
{
    (void)msauth::clearAccount(util::MinecraftDirectories::getRunDirectory());
    msauth::clearSession(client.session);
}

std::unique_ptr<gui::screen::Screen> createLoginScreen(gui::screen::ScreenFactory returnFactory)
{
    return std::make_unique<LoginScreen>(std::move(returnFactory));
}

} // namespace net::minecraft::client::gui::auth
