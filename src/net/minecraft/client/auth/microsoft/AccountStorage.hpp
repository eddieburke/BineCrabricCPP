#pragma once
#include <filesystem>
#include <optional>
#include "net/minecraft/client/auth/microsoft/MicrosoftAuth.hpp"
namespace msauth {
[[nodiscard]] std::optional<MicrosoftAccount> loadAccount(const std::filesystem::path& runDirectory);
[[nodiscard]] bool saveAccount(const std::filesystem::path& runDirectory, const MicrosoftAccount& account);
[[nodiscard]] bool clearAccount(const std::filesystem::path& runDirectory);
} // namespace msauth
