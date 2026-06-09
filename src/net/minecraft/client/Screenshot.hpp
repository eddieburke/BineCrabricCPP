#pragma once

#include <filesystem>
#include <string>

namespace net::minecraft::client {

class Screenshot {
public:
    static std::string take(const std::filesystem::path& gameDir, int width, int height);
};

} // namespace net::minecraft::client
