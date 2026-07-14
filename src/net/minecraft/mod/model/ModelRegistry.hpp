#pragma once
#include <memory>
#include <string>
namespace net::minecraft::mod::model {
struct BakedModel;
// Loads and bakes a JSON model from a mod's assets (resolving parent chains),
// caching by (modId, path). Returns a handle >= 1, or 0 with error set.
int loadBakedModel(const std::string& modId, const std::string& path, std::string& error);
// Cache entry points for external bakers (e.g. voxel sprite extrusion).
[[nodiscard]] int bakedModelHandleForKey(const std::string& key) noexcept;
int storeBakedModel(const std::string& key, std::unique_ptr<BakedModel> baked);
// Looks up a previously loaded model; nullptr for unknown handles.
[[nodiscard]] const BakedModel* bakedModelForHandle(int handle) noexcept;
} // namespace net::minecraft::mod::model
