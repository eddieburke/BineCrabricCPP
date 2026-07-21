#include "net/minecraft/client/render/shaderpack/ShaderPackManager.hpp"
#include "net/minecraft/client/ClientLog.hpp"
#include "net/minecraft/client/gl/GlConstants.hpp"
#include "net/minecraft/client/gl/EnginePipeline.hpp"
#include "net/minecraft/client/gl/GLCore.hpp"
#include "net/minecraft/client/gl/ProgramCache.hpp"
#include "net/minecraft/client/gl/ShaderProgram.hpp"
#include "net/minecraft/client/option/GameOptions.hpp"
#include "net/minecraft/client/render/FrameRenderCamera.hpp"
#include "net/minecraft/client/render/Framebuffer.hpp"
#include "net/minecraft/client/render/RenderSystem.hpp"
#include "net/minecraft/client/render/RenderType.hpp"
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/world/light/UnifiedLightRegistry.hpp"
#include "net/minecraft/client/resource/pack/ZippedTexturePack.hpp"
#include "net/minecraft/client/resource/ResourceRoot.hpp"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <set>
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

constexpr unsigned int kTexture2D = 0x0DE1;

void applyDummyFilters() {
 ::glTexParameteri(kTexture2D, 0x2801, 0x2600);
 ::glTexParameteri(kTexture2D, 0x2800, 0x2600);
 ::glTexParameteri(kTexture2D, 0x2802, 0x812F);
 ::glTexParameteri(kTexture2D, 0x2803, 0x812F);
}

unsigned int zeroSampler2DTexture() {
 static unsigned int texture = 0;
 if(texture == 0) {
  texture = RenderSystem::genTexture();
  RenderSystem::bindTexture(kTexture2D, static_cast<int>(texture));
  const unsigned char pixel[4] = {0, 0, 0, 0};
  ::glTexImage2D(kTexture2D, 0, 0x8058, 1, 1, 0, 0x1908, 0x1401, pixel);
  applyDummyFilters();
 }
 return texture;
}

unsigned int zeroUSampler2DTexture() {
 static unsigned int texture = 0;
 if(texture == 0) {
  texture = RenderSystem::genTexture();
  RenderSystem::bindTexture(kTexture2D, static_cast<int>(texture));
  const unsigned int pixel[1] = {0u};
  ::glTexImage2D(kTexture2D, 0, 0x8236, 1, 1, 0, 0x8D94, 0x1405, pixel);
  applyDummyFilters();
 }
 return texture;
}

unsigned int zeroISampler2DTexture() {
 static unsigned int texture = 0;
 if(texture == 0) {
  texture = RenderSystem::genTexture();
  RenderSystem::bindTexture(kTexture2D, static_cast<int>(texture));
  const int pixel[1] = {0};
  ::glTexImage2D(kTexture2D, 0, 0x8235, 1, 1, 0, 0x8D94, 0x1404, pixel);
  applyDummyFilters();
 }
 return texture;
}

class UnitAllocator {
 public:
 explicit UnitAllocator(int maximum) : maximum_(maximum) {}
 bool bind3D(gl::ShaderProgram& program, const std::string& name, unsigned int texture) {
  if(next_ >= maximum_) {
   return false;
  }
  RenderSystem::activeTexture(gl::tex::Texture0 + next_);
  ::glBindTexture(0x806F, texture);
  program.set1i(name, next_);
  ++next_;
  return true;
 }
 bool bind(gl::ShaderProgram& program, const std::string& name, unsigned int texture,
           gl::ShaderProgram::SamplerKind kind) {
  if(kind == gl::ShaderProgram::SamplerKind::None || next_ >= maximum_) {
   return false;
  }
  unsigned int resolved = texture;
  if(resolved == 0) {
   switch(kind) {
    case gl::ShaderProgram::SamplerKind::Unsigned:
     resolved = zeroUSampler2DTexture();
     break;
    case gl::ShaderProgram::SamplerKind::Integer:
     resolved = zeroISampler2DTexture();
     break;
    default:
     resolved = zeroSampler2DTexture();
     break;
   }
  }
  RenderSystem::activeTexture(gl::tex::Texture0 + next_);
  RenderSystem::bindTexture(resolved);
  program.set1i(name, next_);
  ++next_;
  return true;
 }
 [[nodiscard]] bool exhausted() const {
  return next_ >= maximum_;
 }

 private:
 int maximum_;
 int next_ = 0;
};

int maxTextureUnits() {
 static int units = 0;
 if(units == 0) {
  int queried = 0;
  ::glGetIntegerv(0x8872, &queried);
  units = queried > 0 ? queried : 16;
 }
 return units;
}

render::ColorFormat parseFormat(const std::string& format) {
 return format == "RGBA16F" ? render::ColorFormat::Rgba16F : render::ColorFormat::Rgba8;
}

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
 render::setWorldProgramResolver([this](const std::string& key) { return worldProgram(key); });
}

ShaderPackManager::~ShaderPackManager() {
 render::setWorldProgramResolver(nullptr);
}
ShaderPackManager::Pack::~Pack() = default;

void ShaderPackManager::reload() {
 packs_.clear();
 summaries_.clear();
 activeIndex_ = kNoActivePack;

 const std::filesystem::path directory = gameDirectory_ / "shaderpacks";
 std::error_code ec;
 std::filesystem::create_directories(directory, ec);
 basePack_ = loadDirectoryPack(directory / "vanilla");

 std::vector<std::filesystem::path> archives;
 std::vector<std::filesystem::path> dirs;
 if(std::filesystem::is_directory(directory)) {
  for(const auto& entry : std::filesystem::directory_iterator(directory, ec)) {
   if(ec) {
    break;
   }
   if(entry.is_directory(ec)) {
    dirs.push_back(entry.path());
   } else if(entry.is_regular_file(ec) && lower(entry.path().extension().string()) == ".zip") {
    archives.push_back(entry.path());
   }
  }
 }
 std::sort(dirs.begin(), dirs.end());
 std::sort(archives.begin(), archives.end());
 for(const auto& path : dirs) {
  addDirectoryPack(path);
 }
 for(const auto& path : archives) {
  addZipPack(path);
 }

 const std::string selected = options_ != nullptr ? options_->shaderPack : std::string{};
 if(!selected.empty() && selected != "OFF") {
  for(std::size_t i = 0; i < packs_.size(); ++i) {
   if(packs_[i]->summary.key == selected || packs_[i]->summary.name == selected) {
    activeIndex_ = i;
    break;
   }
  }
 }
 refreshSummaries();
}

void ShaderPackManager::indexPasses(Pack& pack) {
 for(std::size_t i = 0; i < pack.manifest.passes.size(); ++i) {
  const PackPass& pass = pack.manifest.passes[i];
  if(pass.type == "post" && !pass.program.empty()) {
   pack.postPasses.push_back(i);
  } else if(pass.type == "terrain" && !pass.program.empty() && !pack.terrainPass.has_value()) {
   pack.terrainPass = i;
  }
 }
 pack.programs = std::make_unique<gl::ProgramCache>();
}

std::unique_ptr<ShaderPackManager::Pack> ShaderPackManager::loadDirectoryPack(const std::filesystem::path& path) {
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
  indexPasses(*pack);
 }
 if(pack->summary.name.empty()) {
  pack->summary.name = path.filename().string();
 }
 return pack;
}

void ShaderPackManager::addDirectoryPack(const std::filesystem::path& path) {
 packs_.push_back(loadDirectoryPack(path));
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
  indexPasses(*pack);
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
 if(key.empty() || lower(key) == "off" || lower(key) == "none") {
  activeIndex_ = kNoActivePack;
  if(options_ != nullptr) {
   options_->shaderPack.clear();
   options_->save();
  }
  refreshSummaries();
  return true;
 }
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
    pack->compiledPrograms.clear();
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
 const Pack* pack = activePack();
 return pack != nullptr && !pack->postPasses.empty();
}

gl::ShaderProgram* ShaderPackManager::compileProgram(Pack& pack, const std::string& programName) {
 if(!pack.summary.valid || pack.programs == nullptr) {
  return nullptr;
 }
 const auto compiled = pack.compiledPrograms.find(programName);
 if(compiled != pack.compiledPrograms.end()) {
  return compiled->second;
 }
 const auto found = pack.manifest.programs.find(programName);
 if(found == pack.manifest.programs.end()) {
  return nullptr;
 }
 const PackProgram& spec = found->second;
 const std::string& vertex = cachedText(pack, spec.vertex);
 const std::string& fragment = cachedText(pack, spec.fragment);
 if(vertex.empty() || fragment.empty()) {
  return nullptr;
 }
 std::vector<gl::ShaderDefine> defines;
 defines.reserve(pack.settings.size());
 for(const auto& [key, value] : pack.settings) {
  defines.push_back({key, value});
 }
 const std::string cacheKey = programName + "|" + spec.vertex + "|" + spec.fragment;
 gl::ShaderProgram* program = pack.programs->getFromSource(cacheKey, vertex, fragment, kVersion, defines);
 if(program != nullptr) {
  pack.compiledPrograms.emplace(programName, program);
 }
 return program;
}

gl::ShaderProgram* ShaderPackManager::programFromPack(Pack& pack, const std::string& key) {
 if(!pack.summary.valid) {
  return nullptr;
 }
 // Back-compat: a pack that declares its terrain program only via a terrain pass (e.g.
 // acid's "acid_terrain") still satisfies the canonical "gbuffers_terrain" key.
 if(key == "gbuffers_terrain" && pack.terrainPass.has_value()) {
  return compileProgram(pack, pack.manifest.passes[*pack.terrainPass].program);
 }
 if(pack.manifest.programs.count(key) != 0) {
  return compileProgram(pack, key);
 }
 return nullptr;
}

gl::ShaderProgram* ShaderPackManager::worldProgram(const std::string& key) {
 if(Pack* active = activePack()) {
  if(gl::ShaderProgram* program = programFromPack(*active, key)) {
   return program;
  }
 }
 if(basePack_ != nullptr) {
  return programFromPack(*basePack_, key);
 }
 return nullptr;
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

const std::string& ShaderPackManager::cachedText(Pack& pack, const std::string& path) const {
 const auto found = pack.sourceCache.find(path);
 if(found != pack.sourceCache.end()) {
  return found->second;
 }
 return pack.sourceCache.emplace(path, readText(pack, path)).first->second;
}

bool ShaderPackManager::renderPostProcess(int textureId,
                                          int depthTextureId,
                                          int width,
                                          int height,
                                          float tickDelta,
                                          const FrameRenderCamera& camera,
                                          float farPlane,
                                          float worldTime,
                                          const net::minecraft::World* world,
                                          float fogEnd) {
  Pack* pack = activePack();
  if(pack == nullptr || !pack->summary.valid || pack->programs == nullptr || pack->postPasses.empty()) {
   return false;
  }
  if(textureId < 0 || depthTextureId < 0) {
   return false;
  }
  if(!hasGlContext()) {
   return false;
  }
  if(width <= 0 || height <= 0) {
   return false;
  }
  const RenderSystem::StateShadow saved = RenderSystem::getShadow();
  RenderSystem::disableDepthTest();
  RenderSystem::disableCull();
  RenderSystem::disableBlend();
  RenderSystem::depthMask(false);
  std::unordered_map<std::string, int> textures;
  textures["colortex0"] = textureId;
  textures["depthtex0"] = depthTextureId;
  const std::size_t lastIndex = pack->postPasses.back();
  bool ok = false;
  for(std::size_t passIndex : pack->postPasses) {
   if(passIndex >= pack->manifest.passes.size()) {
    continue;
   }
   const PackPass& pass = pack->manifest.passes[passIndex];
   const auto found = pack->manifest.programs.find(pass.program);
   if(found == pack->manifest.programs.end()) {
    logOnce(*pack, "pass '" + pass.name + "' references unknown program '" + pass.program + "'");
    continue;
   }
   const PackProgram& spec = found->second;
   const std::string& vertex = cachedText(*pack, spec.vertex);
   const std::string& fragment = cachedText(*pack, spec.fragment);
   if(vertex.empty() || fragment.empty()) {
    logOnce(*pack, "pass '" + pass.name + "' missing shader source (" + spec.vertex + ", " + spec.fragment + ")");
    continue;
   }
   gl::ShaderProgram* program = nullptr;
   const auto compiled = pack->compiledPrograms.find(pass.program);
   if(compiled != pack->compiledPrograms.end()) {
    program = compiled->second;
   } else {
    std::vector<gl::ShaderDefine> defines;
    defines.reserve(pack->settings.size());
    for(const auto& [key, value] : pack->settings) {
     defines.push_back({key, value});
    }
    const std::string cacheKey = pass.program + "|" + spec.vertex + "|" + spec.fragment;
    program = pack->programs->getFromSource(cacheKey, vertex, fragment, kVersion, defines);
    if(program != nullptr) {
     pack->compiledPrograms.emplace(pass.program, program);
    }
   }
   if(program == nullptr) {
    logOnce(*pack, "pass '" + pass.name + "' program '" + pass.program + "' failed to compile");
    continue;
   }
   const bool isLast = passIndex == lastIndex;
   const bool toScreen = pass.output.empty() || lower(pass.output) == "screen";
   Framebuffer* target = nullptr;
   if(!toScreen) {
    const auto declared = pack->manifest.targets.find(pass.output);
    const render::ColorFormat format =
        declared == pack->manifest.targets.end() ? render::ColorFormat::Rgba8 : parseFormat(declared->second.format);
    const float scale = declared == pack->manifest.targets.end() ? 1.0f : declared->second.scale;
    const int targetWidth = std::max(1, static_cast<int>(std::lround(static_cast<float>(width) * scale)));
    const int targetHeight = std::max(1, static_cast<int>(std::lround(static_cast<float>(height) * scale)));
    Pack::RenderTarget& slot = pack->targets[pass.output];
    const bool readsOwnOutput =
        std::find(pass.inputs.begin(), pass.inputs.end(), pass.output) != pass.inputs.end();
    const int writeIndex = readsOwnOutput ? 1 - slot.front : slot.front;
    if(slot.buffers[writeIndex] == nullptr) {
     slot.buffers[writeIndex] = std::make_unique<Framebuffer>();
    }
    target = slot.buffers[writeIndex].get();
    if(!target->ensure(targetWidth, targetHeight, format)) {
     logOnce(*pack, "pass '" + pass.name + "' could not allocate target '" + pass.output + "'");
     continue;
    }
    slot.front = writeIndex;
    target->bindTarget();
   } else {
    gl::GLCore::bindFramebuffer(0x8D40, 0);
    RenderSystem::viewport(0, 0, width, height);
   }
   program->bind();
   UnitAllocator units(maxTextureUnits());
   for(const std::string& name : program->declaredSamplers()) {
    const auto resolved = textures.find(name);
    if(resolved != textures.end()) {
     units.bind(*program, name, static_cast<unsigned int>(resolved->second), program->samplerKind(name));
     continue;
    }
    logOnce(*pack, "pass '" + pass.name + "' samples '" + name +
                       "' which this engine does not produce — bound a neutral placeholder");
    units.bind(*program, name, 0, program->samplerKind(name));
   }
   if(units.exhausted()) {
    logOnce(*pack, "pass '" + pass.name + "' requested more texture units than the driver exposes");
   }
   program->set1f("uWorldTime", worldTime);
   program->set1f("uPartialTicks", tickDelta);
   program->set2f("uViewport", static_cast<float>(width), static_cast<float>(height));
   program->set1i("uDepthAvailable", 1);
   program->set1i("uNormalAvailable", textures.count("normaltex0") != 0 ? 1 : 0);
   program->set1i("uShadowAvailable", 0);
   program->set1i("uVoxelAvailable", 0);
   program->set1f("uFogEnd", fogEnd);
   program->set3f("uViewRight", camera.viewRightX, camera.viewRightY, camera.viewRightZ);
   program->set3f("uViewUp", camera.viewUpX, camera.viewUpY, camera.viewUpZ);
   program->set3f("uViewForward", camera.viewForwardX, camera.viewForwardY, camera.viewForwardZ);
   program->set3f("uCameraWorld", static_cast<float>(camera.eyeX), static_cast<float>(camera.eyeY),
                  static_cast<float>(camera.eyeZ));
   program->set2f("uProjectionScale", camera.projectionX, camera.projectionY);
   program->set2f("uNearFar", camera.perspectiveNear, farPlane);
   if(world != nullptr) {
    const auto& sun = world->lightRegistry().sun();
    const float svx = sun.directionX * camera.viewRightX + sun.directionY * camera.viewRightY +
                      sun.directionZ * camera.viewRightZ;
    const float svy = sun.directionX * camera.viewUpX + sun.directionY * camera.viewUpY +
                      sun.directionZ * camera.viewUpZ;
    const float svz = -(sun.directionX * camera.viewForwardX + sun.directionY * camera.viewForwardY +
                        sun.directionZ * camera.viewForwardZ);
    program->set3f("uSunDirectionView", svx, svy, svz);
    program->set3f("uSunColor", sun.red, sun.green, sun.blue);
    program->set1f("uSunIntensity", sun.intensity);
   }
   gl::engine_pipeline::RenderPass enginePass{};
   enginePass.programOverride = program;
   gl::engine_pipeline::submitFullscreenPass(enginePass);
   ok = true;
   if(target != nullptr) {
    textures[pass.output] = static_cast<int>(target->colorTexture());
    if(isLast) {
     gl::GLCore::bindFramebuffer(0x8D40, 0);
     RenderSystem::viewport(0, 0, width, height);
     target->blitToScreen(width, height);
     logOnce(*pack, "final pass '" + pass.name + "' writes to '" + pass.output +
                        "' rather than the screen — the result is copied, set output to screen to avoid it");
    }
   }
   RenderSystem::activeTexture(gl::tex::Texture0);
  }
  RenderSystem::setShadow(saved);
  RenderSystem::activeTexture(gl::tex::Texture0);
  return ok;
}

void ShaderPackManager::logOnce(Pack& pack, const std::string& message) const {
 if(pack.logged.insert(message).second) {
  ClientLog::LOGGER.log(LogLevel::Warning, "[shader] " + pack.summary.name + ": " + message);
 }
}

render::ColorFormat ShaderPackManager::sceneColorFormat() const {
 const Pack* pack = activePack();
 if(pack == nullptr || !pack->summary.valid) {
  return render::ColorFormat::Rgba8;
 }
 const auto found = pack->manifest.targets.find("colortex0");
 if(found == pack->manifest.targets.end()) {
  return render::ColorFormat::Rgba8;
 }
 return parseFormat(found->second.format);
}

} // namespace net::minecraft::client::render::shaderpack
