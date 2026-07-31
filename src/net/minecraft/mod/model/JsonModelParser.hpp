#pragma once
#include <string>
#include "net/minecraft/mod/model/JsonModelTypes.hpp"
#include "net/minecraft/util/json/JsonValue.hpp"
namespace net::minecraft::mod::model::detail {
using util::json::JsonValue;
const JsonModelTexture* findTexture(const JsonModel& model, const std::string& key) noexcept;
bool parseJsonModel(const JsonValue& root, JsonModel& out, std::string& error);
bool loadModelFile(const std::string& modId, const std::string& path, JsonModel& out, std::string& error);
void mergeParentModel(JsonModel& child, const JsonModel& parent);
std::string directoryOf(const std::string& path);
std::string normalizeModelPath(std::string path);
std::string parentModelPath(const std::string& parent, const std::string& basePath);
} // namespace net::minecraft::mod::model::detail
