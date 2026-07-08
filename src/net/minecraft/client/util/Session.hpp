#pragma once
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace net::minecraft::client::util {
class Session {
   public:
    Session() = default;

    Session(std::string username, std::string sessionId)
        : username(std::move(username)), sessionId(std::move(sessionId)) {
    }

    [[nodiscard]] static const std::vector<int>& creativeInventory() {
        static const std::vector<int> blocks{1,  4,  45, 3,  5,  17, 18, 50, 44, 20, 48, 6,  37, 38,
                                             39, 40, 12, 13, 19, 35, 16, 15, 14, 42, 41, 47, 46, 49};
        return blocks;
    }

    std::string username;
    std::string sessionId;
    std::string mpPass;
    std::int64_t mpPassExpiresAt = 0;
    std::string skinUrl;
    std::string capeUrl;
};
}  // namespace net::minecraft::client::util
