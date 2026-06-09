#pragma once

#include <string>

namespace net::minecraft::client::auth {

struct JoinServerResult {
    bool ok = false;
    std::string responseLine;
    std::string error;
};

// Mirrors Java ClientNetworkHandler.onHandshake online-mode HTTP verification.
[[nodiscard]] JoinServerResult verifyJoinServer(
    const std::string& username,
    const std::string& sessionId,
    const std::string& serverId);

} // namespace net::minecraft::client::auth
