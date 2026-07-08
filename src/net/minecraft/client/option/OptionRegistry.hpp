#pragma once
#include <optional>
#include <span>
#include <string_view>

#include "net/minecraft/client/option/OptionSpec.hpp"

namespace net::minecraft::client::option {
class OptionRegistry {
   public:
    static void registerAll();
    [[nodiscard]] static std::span<const OptionSpec> all();
    [[nodiscard]] static std::optional<std::size_t> indexOf(std::string_view persistKey);
    [[nodiscard]] static const OptionSpec& at(std::size_t index);
    [[nodiscard]] static std::optional<const OptionSpec*> byKey(std::string_view persistKey);

   private:
    static void registerGroup(std::span<const OptionSpec> specs);
};
}  // namespace net::minecraft::client::option
