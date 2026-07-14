#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include <vector>
namespace net::minecraft::util::json {
[[nodiscard]] std::optional<std::string> stringField(const std::string& text, const std::string& key);
[[nodiscard]] std::optional<int> intField(const std::string& text, const std::string& key);
[[nodiscard]] std::optional<std::int64_t> int64Field(const std::string& text, const std::string& key);
[[nodiscard]] std::optional<bool> boolField(const std::string& text, const std::string& key);
[[nodiscard]] std::optional<std::string> objectField(const std::string& text, const std::string& key);
[[nodiscard]] std::vector<std::string> objectArrayField(const std::string& text, const std::string& key);
[[nodiscard]] std::string escape(const std::string& text);
} // namespace net::minecraft::util::json
