#pragma once

#include <filesystem>
#include <optional>

namespace msauth {

[[nodiscard]] std::optional<std::filesystem::path> pickJsonFile();

} // namespace msauth
