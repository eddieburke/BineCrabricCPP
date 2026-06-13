#pragma once

#include "net/minecraft/client/util/Session.hpp"

#include <string>

namespace net::minecraft::client::auth {

struct JoinServerResult {
    bool ok = false;
    std::string responseLine;
    std::string error;
};

// Mirrors Java ClientNetworkHandler.onHandshake online-mode HTTP verification.
[[nodiscard]] JoinServerResult verifyJoinServer(
    const net::minecraft::client::util::Session& session,
    const std::string& serverId);

} // namespace net::minecraft::client::auth
