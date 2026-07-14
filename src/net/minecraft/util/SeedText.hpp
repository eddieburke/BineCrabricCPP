#pragma once
#include <cstdint>
#include <string>
namespace net::minecraft::util {
[[nodiscard]] std::int32_t javaStringHashCode(const std::string& text);
[[nodiscard]] std::uint64_t resolveSeedText(const std::string& text);
} // namespace net::minecraft::util
