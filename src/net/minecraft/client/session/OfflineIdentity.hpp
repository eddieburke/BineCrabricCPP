#pragma once
#include <string>
#include "net/minecraft/client/util/Session.hpp"
namespace net::minecraft::client::session {
// Runtime identity override for offline-mode (non-authenticated) multiplayer joins.
//
// When a Microsoft session is *not* active, the client normally joins offline servers
// under the launch-time fallback name ("PlayerNNN"). A mod may install an override here
// so the client presents a chosen username to offline servers instead. The override is
// ignored while a real session is authenticated, because authenticated joins are
// name-locked by the session profile.
void setOfflineUsername(std::string name);
void clearOfflineUsername();
[[nodiscard]] bool hasOfflineUsername();
[[nodiscard]] const std::string& offlineUsername();
// Resolves the username to send in the handshake / login-hello for a join. Returns the
// override when one is set and the session is not authenticated, otherwise the live
// session username.
[[nodiscard]] std::string resolveJoinUsername(const util::Session& session);
} // namespace net::minecraft::client::session
