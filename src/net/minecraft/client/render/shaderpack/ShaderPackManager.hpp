#pragma once
#include "net/minecraft/client/render/shaderpack/PackManifest.hpp"
#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
namespace net::minecraft::client::gl {
class ProgramCache;
class ShaderProgram;
} // namespace net::minecraft::client::gl
namespace net::minecraft::client::option {
class GameOptions;
}
namespace net::minecraft::client::render::light {
class UnifiedLightView;
}
namespace net::minecraft::world::light {
class UnifiedLightRegistry;
}
namespace net::minecraft::client::gl {
class FramebufferManager;
}
namespace net::minecraft::client::render {
struct FrameRenderCamera;
}
namespace net::minecraft::client::resource::pack {
class ZippedTexturePack;
}
namespace net::minecraft::client::render::shaderpack {
struct ShaderPackSummary {
  std::string key;
  std::string name;
  std::string version;
  std::string error;
  bool valid = false;
  bool selected = false;
};
class ShaderPackManager {
public:
  ShaderPackManager(std::filesystem::path gameDirectory, option::GameOptions* options);
  ~ShaderPackManager();
  void reload();
  bool select(const std::string& key);
  bool setSetting(const std::string& key, std::string value);
  [[nodiscard]] std::string settingValue(const std::string& key) const;
  [[nodiscard]] const std::vector<ShaderPackSummary>& available() const noexcept { return summaries_; }
  [[nodiscard]] const PackManifest* activeManifest() const noexcept;
  [[nodiscard]] bool activeHasPostProcess() const;
  bool renderPostProcess(int textureId, int depthTextureId, int width, int height, float tickDelta,
                         const FrameRenderCamera& camera, float farPlane, float worldTime,
                         ::net::minecraft::world::light::UnifiedLightRegistry* lightRegistry = nullptr,
                         light::UnifiedLightView* lightView = nullptr,
                         int shadowDepthTextureId = -1,
                         const FrameRenderCamera* shadowCamera = nullptr,
                         gl::FramebufferManager* framebufferManager = nullptr);

private:
  struct Pack {
    ShaderPackSummary summary;
    std::filesystem::path path;
    bool directory = true;
    std::unique_ptr<net::minecraft::client::resource::pack::ZippedTexturePack> zip;
    PackManifest manifest;
    std::unordered_map<std::string, std::string> settings;
    std::unique_ptr<gl::ProgramCache> programs;
  };
  [[nodiscard]] std::string readText(const Pack& pack, const std::string& path) const;
  [[nodiscard]] Pack* activePack() noexcept;
  [[nodiscard]] const Pack* activePack() const noexcept;
  void addDirectoryPack(const std::filesystem::path& path);
  void addZipPack(const std::filesystem::path& path);
  void refreshSummaries();
  std::filesystem::path gameDirectory_;
  option::GameOptions* options_ = nullptr;
  std::vector<std::unique_ptr<Pack>> packs_;
  std::vector<ShaderPackSummary> summaries_;
  std::size_t activeIndex_ = 0;
};
} // namespace net::minecraft::client::render::shaderpack
