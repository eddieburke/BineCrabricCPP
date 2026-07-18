#include "net/minecraft/client/render/shaderpack/ShaderPackManager.hpp"
#include "net/minecraft/client/gl/EnginePipeline.hpp"
#include "net/minecraft/client/gl/GLCore.hpp"
#include "net/minecraft/client/gl/GlState.hpp"
#include "net/minecraft/client/gl/ProgramCache.hpp"
#include "net/minecraft/client/gl/ShaderProgram.hpp"
#include "net/minecraft/client/option/GameOptions.hpp"
#include "net/minecraft/client/render/FrameRenderCamera.hpp"
#include "net/minecraft/client/gl/Framebuffer.hpp"
#include "net/minecraft/client/render/light/UnifiedLightView.hpp"
#include "net/minecraft/world/light/UnifiedLightRegistry.hpp"
#include "net/minecraft/client/resource/pack/ZippedTexturePack.hpp"
#include "net/minecraft/client/resource/ResourceRoot.hpp"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <sstream>
namespace net::minecraft::client::render::shaderpack {
namespace {
constexpr const char* kVersion = "#version 330 core\n";
std::string lower(std::string value) {
  for(char& ch : value) {
    ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
  }
  return value;
}
std::string readFile(const std::filesystem::path& path) {
  std::ifstream input(path, std::ios::binary);
  if(!input) {
    return {};
  }
  std::ostringstream content;
  content << input.rdbuf();
  return content.str();
}
std::string defaultSettingValue(const PackSetting& setting) { return setting.defaultValue; }
bool normalizeSettingValue(const PackSetting& setting, const std::string& input, std::string& output) {
  if(setting.type == SettingType::Bool) {
    const std::string normalized = lower(input);
    if(normalized == "1" || normalized == "true" || normalized == "on") {
      output = "1";
      return true;
    }
    if(normalized == "0" || normalized == "false" || normalized == "off") {
      output = "0";
      return true;
    }
    return false;
  }
  char* end = nullptr;
  const double parsed = std::strtod(input.c_str(), &end);
  if(end == input.c_str() || *end != '\0' || !std::isfinite(parsed)) {
    return false;
  }
  double value = std::clamp(parsed, setting.minimum, setting.maximum);
  value = setting.minimum + std::round((value - setting.minimum) / setting.step) * setting.step;
  value = std::clamp(value, setting.minimum, setting.maximum);
  if(setting.type == SettingType::Int) {
    output = std::to_string(static_cast<int>(std::lround(value)));
  } else {
    output = std::to_string(value);
  }
  return true;
}
bool hasGlContext() {
#ifdef _WIN32
  return wglGetCurrentContext() != nullptr;
#else
  return gl::GLCore::activeTexture != nullptr;
#endif
}
} // namespace
ShaderPackManager::ShaderPackManager(std::filesystem::path gameDirectory, option::GameOptions* options)
    : gameDirectory_(std::move(gameDirectory)), options_(options) {
  reload();
}
ShaderPackManager::~ShaderPackManager() = default;
void ShaderPackManager::reload() {
  packs_.clear();
  summaries_.clear();
  activeIndex_ = 0;
  const std::filesystem::path directory = gameDirectory_ / "shaderpacks";
  std::error_code ec;
  std::filesystem::create_directories(directory, ec);
  std::vector<std::filesystem::path> directories;
  std::vector<std::filesystem::path> archives;
  if(std::filesystem::is_directory(directory)) {
    for(const auto& entry : std::filesystem::directory_iterator(directory, ec)) {
      if(ec) {
        break;
      }
      if(entry.is_directory(ec)) {
        directories.push_back(entry.path());
      } else if(entry.is_regular_file(ec) && lower(entry.path().extension().string()) == ".zip") {
        archives.push_back(entry.path());
      }
    }
  }
  std::sort(directories.begin(), directories.end());
  std::sort(archives.begin(), archives.end());
  for(const auto& path : directories) {
    addDirectoryPack(path);
  }
  for(const auto& path : archives) {
    addZipPack(path);
  }
  const std::filesystem::path bundledRoot = resource::resourceRoot() / "shaderpacks";
  if(std::filesystem::is_directory(bundledRoot, ec)) {
    for(const auto& entry : std::filesystem::directory_iterator(bundledRoot, ec)) {
      if(ec) {
        break;
      }
      if(!entry.is_directory(ec)) {
        continue;
      }
      const std::string key = lower(entry.path().filename().string());
      const bool localWins = std::any_of(directories.begin(), directories.end(), [&](const std::filesystem::path& path) {
        return lower(path.filename().string()) == key;
      }) || std::any_of(archives.begin(), archives.end(), [&](const std::filesystem::path& path) {
        return lower(path.stem().string()) == key;
      });
      if(!localWins) {
        addDirectoryPack(entry.path());
      }
    }
  }
  const std::string selected = options_ != nullptr ? options_->shaderPack : std::string{};
  for(std::size_t i = 0; i < packs_.size(); ++i) {
    if(packs_[i]->summary.key == selected || packs_[i]->summary.name == selected) {
      activeIndex_ = i;
      break;
    }
  }
  refreshSummaries();
}
void ShaderPackManager::addDirectoryPack(const std::filesystem::path& path) {
  auto pack = std::make_unique<Pack>();
  pack->path = path;
  pack->directory = true;
  pack->summary.key = path.filename().string();
  const std::string text = readFile(path / "pack.json");
  if(text.empty() || !PackManifest::parse(text, pack->manifest, pack->summary.error)) {
    if(pack->summary.error.empty()) {
      pack->summary.error = "pack.json not found";
    }
  } else {
    pack->summary.valid = true;
    pack->summary.name = pack->manifest.name;
    pack->summary.version = pack->manifest.version;
    for(const PackSetting& setting : pack->manifest.settings) {
      pack->settings.emplace(setting.key, defaultSettingValue(setting));
    }
    pack->programs = std::make_unique<gl::ProgramCache>();
  }
  if(pack->summary.name.empty()) {
    pack->summary.name = path.filename().string();
  }
  packs_.push_back(std::move(pack));
}
void ShaderPackManager::addZipPack(const std::filesystem::path& path) {
  auto pack = std::make_unique<Pack>();
  pack->path = path;
  pack->directory = false;
  pack->summary.key = path.filename().string();
  pack->zip = std::make_unique<resource::pack::ZippedTexturePack>(path);
  pack->zip->open();
  const std::vector<std::uint8_t> bytes = pack->zip->getResource("pack.json");
  const std::string text(bytes.begin(), bytes.end());
  if(text.empty() || !PackManifest::parse(text, pack->manifest, pack->summary.error)) {
    if(pack->summary.error.empty()) {
      pack->summary.error = "pack.json not found";
    }
  } else {
    pack->summary.valid = true;
    pack->summary.name = pack->manifest.name;
    pack->summary.version = pack->manifest.version;
    for(const PackSetting& setting : pack->manifest.settings) {
      pack->settings.emplace(setting.key, defaultSettingValue(setting));
    }
    pack->programs = std::make_unique<gl::ProgramCache>();
  }
  if(pack->summary.name.empty()) {
    pack->summary.name = path.filename().string();
  }
  packs_.push_back(std::move(pack));
}
void ShaderPackManager::refreshSummaries() {
  summaries_.clear();
  summaries_.reserve(packs_.size());
  for(std::size_t i = 0; i < packs_.size(); ++i) {
    ShaderPackSummary summary = packs_[i]->summary;
    summary.selected = i == activeIndex_;
    summaries_.push_back(std::move(summary));
  }
}
bool ShaderPackManager::select(const std::string& key) {
  for(std::size_t i = 0; i < packs_.size(); ++i) {
    if(packs_[i]->summary.key != key && packs_[i]->summary.name != key) {
      continue;
    }
    if(!packs_[i]->summary.valid) {
      return false;
    }
    activeIndex_ = i;
    if(options_ != nullptr) {
      options_->shaderPack = packs_[i]->summary.key;
      options_->save();
    }
    refreshSummaries();
    return true;
  }
  return false;
}
bool ShaderPackManager::setSetting(const std::string& key, std::string value) {
  Pack* pack = activePack();
  if(pack == nullptr || pack->manifest.settings.empty()) {
    return false;
  }
  for(const PackSetting& setting : pack->manifest.settings) {
    if(setting.key == key) {
      std::string normalized;
      if(!normalizeSettingValue(setting, value, normalized)) {
        return false;
      }
      pack->settings[key] = std::move(normalized);
      if(pack->programs != nullptr) {
        pack->programs->clear();
      }
      return true;
    }
  }
  return false;
}
std::string ShaderPackManager::settingValue(const std::string& key) const {
  const Pack* pack = activePack();
  if(pack == nullptr) {
    return {};
  }
  const auto found = pack->settings.find(key);
  return found == pack->settings.end() ? std::string() : found->second;
}
ShaderPackManager::Pack* ShaderPackManager::activePack() noexcept {
  return activeIndex_ < packs_.size() ? packs_[activeIndex_].get() : nullptr;
}
const ShaderPackManager::Pack* ShaderPackManager::activePack() const noexcept {
  return activeIndex_ < packs_.size() ? packs_[activeIndex_].get() : nullptr;
}
const PackManifest* ShaderPackManager::activeManifest() const noexcept {
  const Pack* pack = activePack();
  return pack != nullptr && pack->summary.valid ? &pack->manifest : nullptr;
}
bool ShaderPackManager::activeHasPostProcess() const {
  const PackManifest* manifest = activeManifest();
  if(manifest == nullptr) {
    return false;
  }
  return std::any_of(manifest->passes.begin(), manifest->passes.end(),
                     [](const PackPass& pass) { return pass.type == "post" && !pass.program.empty(); });
}
std::string ShaderPackManager::readText(const Pack& pack, const std::string& path) const {
  const std::filesystem::path normalized = std::filesystem::path(path).lexically_normal();
  if(normalized.empty() || normalized.is_absolute() ||
     std::any_of(normalized.begin(), normalized.end(), [](const std::filesystem::path& part) { return part == ".."; })) {
    return {};
  }
  if(pack.directory) {
    return readFile(pack.path / normalized);
  }
  if(pack.zip == nullptr) {
    return {};
  }
  const std::vector<std::uint8_t> bytes = pack.zip->getResource(normalized.generic_string());
  return std::string(bytes.begin(), bytes.end());
}
bool ShaderPackManager::renderPostProcess(int textureId,
                                          int depthTextureId,
                                          int width,
                                          int height,
                                          float tickDelta,
                                          const FrameRenderCamera& camera,
                                          float farPlane,
                                          float worldTime,
                                          ::net::minecraft::world::light::UnifiedLightRegistry* lightRegistry,
                                          light::UnifiedLightView* lightView,
                                          int shadowDepthTextureId,
                                          const FrameRenderCamera* shadowCamera,
                                          gl::FramebufferManager* framebufferManager) {
  if(!hasGlContext()) {
    return false;
  }
  Pack* pack = activePack();
  if(pack == nullptr || !pack->summary.valid || pack->programs == nullptr) {
    return false;
  }
  std::vector<const PackPass*> passes;
  for(const PackPass& pass : pack->manifest.passes) {
    if(pass.type == "post" && !pass.program.empty()) {
      passes.push_back(&pass);
    }
  }
  if(passes.empty()) {
    return false;
  }
  std::vector<gl::ShaderDefine> defines;
  defines.reserve(pack->settings.size());
  for(const auto& [key, value] : pack->settings) {
    defines.push_back({key, value});
  }
  std::unordered_map<std::string, int> targets;
  int currentTexture = textureId;
  for(std::size_t passIndex = 0; passIndex < passes.size(); ++passIndex) {
    const PackPass& post = *passes[passIndex];
    const auto found = pack->manifest.programs.find(post.program);
    if(found == pack->manifest.programs.end()) {
      return false;
    }
    const PackProgram& spec = found->second;
    const std::string vertex = readText(*pack, spec.vertex);
    const std::string fragment = readText(*pack, spec.fragment);
    if(vertex.empty() || fragment.empty()) {
      return false;
    }
    const std::string cacheKey = post.program + "|" + spec.vertex + "|" + spec.fragment;
    gl::ShaderProgram* program = pack->programs->getFromSource(cacheKey, vertex, fragment, kVersion, defines);
    if(program == nullptr) {
      return false;
    }
    const bool finalPass = passIndex + 1 == passes.size() || post.output.empty() || post.output == "screen";
    int outputHandle = -1;
    if(!finalPass) {
      if(framebufferManager == nullptr) {
        return false;
      }
      const auto existing = targets.find(post.output);
      if(existing != targets.end()) {
        outputHandle = existing->second;
      } else {
        const auto target = pack->manifest.targets.find(post.output);
        const bool hdr = target != pack->manifest.targets.end() && target->second.format == "RGBA16F";
        const float scale = target != pack->manifest.targets.end() ? target->second.scale : 1.0f;
        const int targetWidth = std::max(1, static_cast<int>(std::lround(width * scale)));
        const int targetHeight = std::max(1, static_cast<int>(std::lround(height * scale)));
        outputHandle = framebufferManager->create(targetWidth, targetHeight, 1, false, hdr);
        if(outputHandle < 0) {
          return false;
        }
        targets.emplace(post.output, outputHandle);
      }
      if(!framebufferManager->bind(outputHandle)) {
        return false;
      }
    } else {
      gl::FramebufferManager::unbind();
    }
    const int outputWidth = outputHandle >= 0 ? framebufferManager->width(outputHandle) : width;
    const int outputHeight = outputHandle >= 0 ? framebufferManager->height(outputHandle) : height;
    gl::viewport(0, 0, outputWidth, outputHeight);
    gl::activeTexture(gl::tex::Texture0);
    gl::bindTexture(gl::cap::Texture2D, currentTexture);
    if(depthTextureId > 0) {
      gl::activeTexture(gl::tex::Texture0 + 1);
      gl::bindTexture(gl::cap::Texture2D, depthTextureId);
      gl::activeTexture(gl::tex::Texture0);
    }
    program->bind();
    program->set1f("uPartialTicks", tickDelta);
    program->set2f("uViewport", static_cast<float>(outputWidth), static_cast<float>(outputHeight));
    program->set3f("uViewRight", camera.viewRightX, camera.viewRightY, camera.viewRightZ);
    program->set3f("uViewUp", camera.viewUpX, camera.viewUpY, camera.viewUpZ);
    program->set3f("uViewForward", camera.viewForwardX, camera.viewForwardY, camera.viewForwardZ);
    program->set2f("uProjectionScale", camera.projectionX, camera.projectionY);
    program->set2f("uNearFar", std::max(0.001f, camera.perspectiveNear), std::max(farPlane, camera.perspectiveNear + 0.001f));
    program->set1f("uWorldTime", worldTime);
    program->set1i("uDepthAvailable", depthTextureId > 0 ? 1 : 0);
  float sunX = 0.0f;
  float sunY = 1.0f;
  float sunZ = 0.0f;
  float sunRed = 1.0f;
  float sunGreen = 1.0f;
  float sunBlue = 1.0f;
  float sunIntensity = 0.0f;
    if(lightRegistry != nullptr) {
    auto registryView = lightRegistry->read();
    const ::net::minecraft::world::light::PhysicalLight* sun =
        registryView.find(::net::minecraft::world::light::UnifiedLightRegistry::sunKey());
    if(sun != nullptr) {
      sunX = sun->directionX;
      sunY = sun->directionY;
      sunZ = sun->directionZ;
      sunRed = sun->red;
      sunGreen = sun->green;
      sunBlue = sun->blue;
      sunIntensity = sun->intensity;
    }
    }
  const float sunViewX = sunX * camera.viewRightX + sunY * camera.viewRightY + sunZ * camera.viewRightZ;
  const float sunViewY = sunX * camera.viewUpX + sunY * camera.viewUpY + sunZ * camera.viewUpZ;
  const float sunViewZ = -(sunX * camera.viewForwardX + sunY * camera.viewForwardY + sunZ * camera.viewForwardZ);
    program->set3f("uSunDirectionView", sunViewX, sunViewY, sunViewZ);
    program->set3f("uSunColor", sunRed, sunGreen, sunBlue);
    program->set1f("uSunIntensity", sunIntensity);
    const bool lightsBound = lightView != nullptr && lightView->bind(*program, 2, camera, outputWidth, outputHeight);
    program->set1i("lightData0", 2);
    program->set1i("lightData1", 3);
    program->set1i("lightCount", lightsBound ? lightView->count() : 0);
    program->set1f("lightsCount", static_cast<float>(lightsBound ? lightView->count() : 0));
    const bool shadowBound = shadowDepthTextureId > 0 && shadowCamera != nullptr;
    if(shadowBound) {
    gl::activeTexture(gl::tex::Texture0 + 6);
    gl::bindTexture(gl::cap::Texture2D, shadowDepthTextureId);
    gl::activeTexture(gl::tex::Texture0);
    }
    program->set1i("shadowtex0", 6);
    program->set1i("uShadowAvailable", shadowBound ? 1 : 0);
    program->set3f("uCameraWorld", static_cast<float>(camera.eyeX), static_cast<float>(camera.eyeY),
                 static_cast<float>(camera.eyeZ));
    if(shadowCamera != nullptr) {
    program->set3f("uShadowCameraWorld", static_cast<float>(shadowCamera->eyeX),
                   static_cast<float>(shadowCamera->eyeY), static_cast<float>(shadowCamera->eyeZ));
    program->set3f("uShadowViewRight", shadowCamera->viewRightX, shadowCamera->viewRightY,
                   shadowCamera->viewRightZ);
    program->set3f("uShadowViewUp", shadowCamera->viewUpX, shadowCamera->viewUpY, shadowCamera->viewUpZ);
    program->set3f("uShadowViewForward", shadowCamera->viewForwardX, shadowCamera->viewForwardY,
                   shadowCamera->viewForwardZ);
    program->set2f("uShadowOrthoHalf", shadowCamera->orthoHalfWidth, shadowCamera->orthoHalfHeight);
    program->set2f("uShadowNearFar", shadowCamera->orthoNear, shadowCamera->orthoFar);
    }
    if(!gl::engine_pipeline::drawFullscreen(*program, currentTexture, depthTextureId)) {
      return false;
    }
    if(!finalPass) {
      currentTexture = framebufferManager->textureId(outputHandle);
    }
  }
  for(const auto& [name, handle] : targets) {
    (void)name;
    framebufferManager->destroy(handle);
  }
  return true;
}
} // namespace net::minecraft::client::render::shaderpack
