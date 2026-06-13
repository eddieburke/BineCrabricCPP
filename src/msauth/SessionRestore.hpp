#pragma once

#include "msauth/MicrosoftAuth.hpp"
#include "net/minecraft/client/util/Session.hpp"

namespace net::minecraft::client {
class Minecraft;
}

namespace msauth {

[[nodiscard]] bool isAuthenticated(const net::minecraft::client::util::Session& session);
void applyAccount(const MicrosoftAccount& account, net::minecraft::client::util::Session& session);
void clearSession(net::minecraft::client::util::Session& session);

void tryApplySavedAccount(net::minecraft::client::Minecraft& client);

} // namespace msauth
