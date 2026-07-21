#pragma once
#include "net/minecraft/client/render/shaderpack/PackManifest.hpp"
#include <filesystem>
#include <memory>
#include <optional>
#include <set>
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

namespace net::minecraft::client::render {
class Framebuffer;
class ShaderTextureSource;
enum class ColorFormat;
struct FrameRenderCamera;
}

namespace net::minecraft {
class World;
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
  [[nodiscard]] bool active() const noexcept { return activeIndex_ < packs_.size(); }
  bool setSetting(const std::string& key, std::string value);
  [[nodiscard]] std::string settingValue(const std::string& key) const;
  [[nodiscard]] const std::vector<ShaderPackSummary>& available() const noexcept { return summaries_; }
  [[nodiscard]] const PackManifest* activeManifest() const noexcept;
  [[nodiscard]] bool activeHasPostProcess() const;
  [[nodiscard]] render::ColorFormat sceneColorFormat() const;
  // Resolves a canonical world program key ("gbuffers_terrain", "gbuffers_entities")
  // through the active user pack, then the always-loaded vanilla base pack. Returns the
  // compiled program (unbound; EnginePipeline uploads its uniforms), or nullptr. This is
  // the single entry point the RenderType world program resolver calls.
  gl::ShaderProgram* worldProgram(const std::string& key);
   bool renderPostProcess(int textureId, int depthTextureId, int width, int height, float tickDelta,
                          const FrameRenderCamera& camera, float farPlane, float worldTime,
                          const net::minecraft::World* world,
                          float fogEnd = 0.0f);

 private:
  struct Pack {
   ~Pack();
   ShaderPackSummary summary;
   std::filesystem::path path;
   bool directory = false;
   std::unique_ptr<net::minecraft::client::resource::pack::ZippedTexturePack> zip;
   PackManifest manifest;
   std::unordered_map<std::string, std::string> settings;
   std::unordered_map<std::string, std::string> sourceCache;
   std::unordered_map<std::string, gl::ShaderProgram*> compiledPrograms;
    std::vector<std::size_t> postPasses;
    std::optional<std::size_t> terrainPass;
    std::unique_ptr<gl::ProgramCache> programs;
    struct RenderTarget {
     std::unique_ptr<net::minecraft::client::render::Framebuffer> buffers[2];
     int front = 0;
    };
    std::unordered_map<std::string, RenderTarget> targets;
    std::set<std::string> logged;
   };
  void logOnce(Pack& pack, const std::string& message) const;
  [[nodiscard]] std::string readText(const Pack& pack, const std::string& path) const;
  [[nodiscard]] const std::string& cachedText(Pack& pack, const std::string& path) const;
  [[nodiscard]] Pack* activePack() noexcept;
  [[nodiscard]] const Pack* activePack() const noexcept;
  std::unique_ptr<Pack> loadDirectoryPack(const std::filesystem::path& path);
  void addDirectoryPack(const std::filesystem::path& path);
  void addZipPack(const std::filesystem::path& path);
  static void indexPasses(Pack& pack);
  gl::ShaderProgram* programFromPack(Pack& pack, const std::string& key);
  gl::ShaderProgram* compileProgram(Pack& pack, const std::string& programName);
  void refreshSummaries();
  std::filesystem::path gameDirectory_;
  option::GameOptions* options_ = nullptr;
  std::vector<std::unique_ptr<Pack>> packs_;
  std::unique_ptr<Pack> basePack_;
  std::vector<ShaderPackSummary> summaries_;
  static constexpr std::size_t kNoActivePack = static_cast<std::size_t>(-1);
  std::size_t activeIndex_ = kNoActivePack;
};

} // namespace net::minecraft::client::render::shaderpack
