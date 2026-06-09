#pragma once

#include <regex>
#include <string>

namespace net::minecraft {

class DataFilenameFilter {
public:
    static inline const std::regex PATTERN = std::regex(R"(c\.(-?[0-9a-z]+)\.(-?[0-9a-z]+)\.dat)");

    [[nodiscard]] static bool accept(const std::string& filename)
    {
        return std::regex_match(filename, PATTERN);
    }

private:
    DataFilenameFilter() = default;
};

} // namespace net::minecraft
