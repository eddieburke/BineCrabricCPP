#pragma once

#include "msauth/MicrosoftAuth.hpp"
#include "net/minecraft/client/util/Session.hpp"

#include <filesystem>
#include <optional>
#include <string>

namespace net::minecraft::client {
class Minecraft;
}

namespace msauth {

[[nodiscard]] bool isAuthenticated(const net::minecraft::client::util::Session& session);
void applyAccount(const MicrosoftAccount& account, net::minecraft::client::util::Session& session);
void clearSession(net::minecraft::client::util::Session& session);

[[nodiscard]] bool hasRestorableSavedAccount(const std::filesystem::path& runDirectory);
[[nodiscard]] std::optional<std::string> savedAccountProfileName(const std::filesystem::path& runDirectory);
[[nodiscard]] bool isSavedAccountRestorePending();

// Blocking refresh from msauth-account.json (multiplayer join, etc.).
void tryApplySavedAccount(net::minecraft::client::Minecraft& client);

// Non-blocking startup restore; call tickRestoreSavedAccount from the main loop.
void beginRestoreSavedAccount(net::minecraft::client::Minecraft& client);
void tickRestoreSavedAccount(net::minecraft::client::Minecraft& client);

} // namespace msauth
