#pragma once
#include <cstdint>
namespace net::minecraft::client::option {
enum class ApplyFlags : std::uint16_t {
 None = 0,
 ReloadWorld = 1 << 0,
 ReloadTextures = 1 << 1,
 ApplyDerived = 1 << 2,
 ApplyToWorld = 1 << 3,
 UpdateSound = 1 << 4,
};
constexpr ApplyFlags operator|(ApplyFlags a, ApplyFlags b) {
 return static_cast<ApplyFlags>(static_cast<std::uint16_t>(a) | static_cast<std::uint16_t>(b));
}
constexpr ApplyFlags operator&(ApplyFlags a, ApplyFlags b) {
 return static_cast<ApplyFlags>(static_cast<std::uint16_t>(a) & static_cast<std::uint16_t>(b));
}
} // namespace net::minecraft::client::option
