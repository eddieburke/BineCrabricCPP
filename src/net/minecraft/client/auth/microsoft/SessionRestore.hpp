#pragma once
#include <filesystem>
#include <optional>
#include <string>
#include "net/minecraft/client/auth/microsoft/MicrosoftAuth.hpp"
#include "net/minecraft/client/util/Session.hpp"
namespace net::minecraft::client {
class Minecraft;
}
namespace msauth {
[[nodiscard]] bool isAuthenticated(const net::minecraft::client::util::Session& session);
void applyAccount(const MicrosoftAccount& account, net::minecraft::client::util::Session& session);
void applyAccountToClient(net::minecraft::client::Minecraft& client, const MicrosoftAccount& account);
void clearSession(net::minecraft::client::util::Session& session);
[[nodiscard]] bool isSavedAccountRestorePending();
[[nodiscard]] AuthStage savedAccountRestoreStage();
// Requests cancellation of the startup refresh and discards any unapplied result.
void cancelSavedAccountRestore();
// Blocking: ensure the session is authenticated from the saved account before a
// multiplayer join. Waits out any in-flight async startup restore so the rotating
// refresh token is not redeemed twice concurrently. Returns true when authenticated
// or when there is no saved account (offline join allowed); returns false only when a
// saved account exists but could not be restored - the caller must then refuse the
// connect instead of joining under the launch fallback name.
[[nodiscard]] bool ensureAuthenticatedForJoin(net::minecraft::client::Minecraft& client,
                                              const std::atomic_bool* canceled = nullptr);
// Non-blocking startup restore; call tickRestoreSavedAccount from the main loop.
void beginRestoreSavedAccount(net::minecraft::client::Minecraft& client);
void tickRestoreSavedAccount(net::minecraft::client::Minecraft& client);
[[nodiscard]] std::optional<std::string> lastSavedAccountRestoreError();
[[nodiscard]] std::optional<MicrosoftAccount> pendingProfileCreationAccount();
[[nodiscard]] bool hasPendingProfileCreationAccount();
} // namespace msauth
