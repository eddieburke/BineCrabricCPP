#pragma once

#include "msauth/MicrosoftAuth.hpp"

#include <filesystem>
#include <optional>

namespace msauth {

[[nodiscard]] std::optional<MicrosoftAccount> loadAccount(const std::filesystem::path& runDirectory);
[[nodiscard]] bool saveAccount(const std::filesystem::path& runDirectory, const MicrosoftAccount& account);
[[nodiscard]] bool clearAccount(const std::filesystem::path& runDirectory);

[[nodiscard]] std::optional<MicrosoftAccount> importAccountFromJsonText(const std::string& json);
[[nodiscard]] std::optional<MicrosoftAccount> importAccountFromJsonFile(const std::filesystem::path& path);

} // namespace msauth
