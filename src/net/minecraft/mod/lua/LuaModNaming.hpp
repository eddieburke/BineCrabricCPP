#pragma once
#include <string>
#include <string_view>
namespace net::minecraft::mod::lua {
[[nodiscard]] std::string toSnakeCase(std::string_view raw);
[[nodiscard]] std::string humanizeTranslationKey(std::string_view raw);
} // namespace net::minecraft::mod::lua
