#pragma once
#include <string>
#include <string_view>
namespace net::minecraft::mod::lua {
// Lowercases and strips whitespace so name lookups (register_block/register_item
// aliases) are forgiving about case and spacing.
[[nodiscard]] std::string normalizeNameToken(std::string_view token);
// camelCase/dotted/underscored -> snake_case, used to derive a stable wire name
// from a translation key.
[[nodiscard]] std::string toSnakeCase(std::string_view raw);
// Turns a translation key into a human-readable display name fallback
// ("iron_bars.name" -> "Iron Bars Name").
[[nodiscard]] std::string humanizeTranslationKey(std::string_view raw);
} // namespace net::minecraft::mod::lua
