#pragma once

#include <filesystem>
#include <regex>
#include <string>

namespace net::minecraft {

namespace fs = std::filesystem;

class DimensionFileFilter {
public:
    static inline const std::regex PATTERN = std::regex(R"([0-9a-z]|([0-9a-z][0-9a-z]))");

    [[nodiscard]] static bool accept(const fs::path& file)
    {
        return std::regex_match(file.filename().string(), PATTERN);
    }

private:
    DimensionFileFilter() = default;
};

} // namespace net::minecraft
