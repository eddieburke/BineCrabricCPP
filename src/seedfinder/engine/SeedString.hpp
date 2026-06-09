#pragma once

#include <cstdint>
#include <string>

namespace seedfinder {

// Duplicate of Java String.hashCode / CodecMus::javaStringHashCode (engine-only).
[[nodiscard]] inline std::int32_t javaStringHashCode(const std::string& s)
{
    std::int32_t h = 0;
    for (unsigned char c : s) {
        h = 31 * h + static_cast<std::int32_t>(c);
    }
    return h;
}

[[nodiscard]] inline std::uint64_t parseSeedToken(const std::string& token)
{
    if (token.empty()) {
        return 0;
    }
    bool allDigits = true;
    bool hasAlpha = false;
    for (char c : token) {
        if (c < '0' || c > '9') {
            allDigits = false;
        }
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) {
            hasAlpha = true;
        }
    }
    if (hasAlpha || (!allDigits && token[0] != '-')) {
        const std::int32_t h = javaStringHashCode(token);
        return static_cast<std::uint64_t>(static_cast<std::int64_t>(h));
    }
    if (token[0] == '-') {
        const std::int64_t v = std::stoll(token);
        return static_cast<std::uint64_t>(v);
    }
    return std::stoull(token);
}

} // namespace seedfinder
