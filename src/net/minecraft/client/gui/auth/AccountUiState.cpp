#include "net/minecraft/client/gui/auth/AccountUiState.hpp"
#include "net/minecraft/client/auth/microsoft/AccountStorage.hpp"
#include "net/minecraft/client/auth/microsoft/SessionRestore.hpp"
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/gui/auth/LoginScreen.hpp"
#include "net/minecraft/client/util/MinecraftDirectories.hpp"
namespace net::minecraft::client::gui::auth {
AccountUiSnapshot pollAccountUi(const Minecraft& client) {
  AccountUiSnapshot snapshot;
  const std::filesystem::path runDirectory = util::MinecraftDirectories::getRunDirectory();
  const std::optional<msauth::MicrosoftAccount> savedAccount = msauth::loadAccount(runDirectory);
  const bool hasSavedAccount = savedAccount.has_value() && savedAccount->restorable();
  snapshot.multiplayerReady = msauth::isAuthenticated(client.session);
  snapshot.showSignOutButton = snapshot.multiplayerReady || hasSavedAccount;
  snapshot.buttonLabel = snapshot.showSignOutButton ? "Sign out" : "Sign in";
  if(msauth::isAuthenticated(client.session)) {
    snapshot.statusLine = "Signed in as " + client.session.username;
    return snapshot;
  }
  if(msauth::isSavedAccountRestorePending()) {
    snapshot.multiplayerReady = false;
    snapshot.statusLine = msauth::authStageMessage(msauth::savedAccountRestoreStage());
    if(snapshot.statusLine.empty()) {
      snapshot.statusLine = "Restoring account...";
    }
    return snapshot;
  }
  if(const std::optional<std::string> restoreError = msauth::lastSavedAccountRestoreError()) {
    snapshot.multiplayerReady = false;
    snapshot.showSignOutButton = false;
    snapshot.buttonLabel = msauth::hasPendingProfileCreationAccount() ? "Finish setup" : "Sign in again";
    snapshot.statusLine = *restoreError;
    return snapshot;
  }
  if(hasSavedAccount) {
    snapshot.statusLine = "Saved account: " + savedAccount->profileName;
  }
  return snapshot;
}
void signOutAccount(Minecraft& client) {
  msauth::cancelSavedAccountRestore();
  (void)msauth::clearAccount(util::MinecraftDirectories::getRunDirectory());
  msauth::clearSession(client.session);
}
std::unique_ptr<gui::screen::Screen> createLoginScreen(gui::screen::ScreenFactory returnFactory) {
  return std::make_unique<LoginScreen>(std::move(returnFactory));
}
} // namespace net::minecraft::client::gui::auth
