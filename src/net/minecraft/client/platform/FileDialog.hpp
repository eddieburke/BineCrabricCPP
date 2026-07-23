#pragma once
// Native file-selection dialogs shared by client features and runtime mods.
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
namespace net::minecraft::client::platform {
[[nodiscard]] std::optional<std::filesystem::path> pickJsonFile();
[[nodiscard]] std::optional<std::filesystem::path> pickFile(std::string_view filterLabel,
                                                            std::string_view filterPattern);
} // namespace net::minecraft::client::platform
