#include "net/minecraft/mod/lua/LuaBlockModel.hpp"
#include "net/minecraft/mod/lua/LuaBlockRegistry.hpp"
#include "net/minecraft/mod/lua/LuaChunkContext.hpp"
#include "net/minecraft/mod/lua/LuaGameApi.hpp"
#include "net/minecraft/mod/lua/LuaHostApi.hpp"
#include "net/minecraft/mod/lua/LuaRuntimePrelude.hpp"
#include "net/minecraft/mod/lua/LuaNbtCodec.hpp"
#include "net/minecraft/mod/runtime/ModRenderScope.hpp"
#include "net/minecraft/mod/runtime/ModHost.hpp"
#include "net/minecraft/mod/runtime/WorldRequiredMods.hpp"
#include "net/minecraft/mod/GameHooks.hpp"
#include "net/minecraft/mod/HookBus.hpp"
#include "net/minecraft/mod/ModLifecycle.hpp"
#include "net/minecraft/mod/ScreenUi.hpp"
#ifdef MINECRAFT_NATIVE_EXPORTS
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/font/TextRenderer.hpp"
#include "net/minecraft/client/gl/GL11.hpp"
#include "net/minecraft/client/gui/layout/ScreenLayout.hpp"
#include "net/minecraft/client/input/InputSystem.hpp"
#include "net/minecraft/client/render/platform/Lighting.hpp"
#include "net/minecraft/client/gui/Draw2D.hpp"
#include "net/minecraft/client/render/Tessellator.hpp"
#include "net/minecraft/client/render/item/ItemRenderer.hpp"
#include "net/minecraft/entity/Entity.hpp"
#include "net/minecraft/entity/player/ClientPlayerEntity.hpp"
#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/item/Item.hpp"
#include "net/minecraft/client/gui/screen/Screen.hpp"
#include "net/minecraft/client/gui/widget/ActionButtonWidget.hpp"
#include "net/minecraft/client/gui/widget/TextFieldWidget.hpp"
#include "net/minecraft/mod/HostScreenRegistry.hpp"
#include "net/minecraft/client/option/ResolvedRenderOptions.hpp"
#include "net/minecraft/client/option/GameOptions.hpp"
#include "net/minecraft/client/option/OptionRegistry.hpp"
#include "net/minecraft/client/option/KeyBinding.hpp"
#include "net/minecraft/client/texture/TextureManager.hpp"
#include "net/minecraft/client/util/MinecraftDirectories.hpp"
#include "net/minecraft/mod/lua/LuaModApi.hpp"
#include "net/minecraft/util/SeedText.hpp"
#endif
#include "net/minecraft/nbt/Compression.hpp"
#include "net/minecraft/nbt/Nbt.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/util/json/JsonFields.hpp"
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/world/gen/GenerationApi.hpp"
#include <algorithm>
#include <atomic>
#include <bit>
#include <chrono>
#include <cmath>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <initializer_list>
#include <iostream>
#include <memory>
#include <mutex>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>
#include <unordered_map>
#include <vector>
#ifdef _WIN32
#include <windows.h>
#endif
namespace net::minecraft::mod::runtime {
namespace {
using namespace net::minecraft::mod::lua;
ModHost::LoadedLuaMod* currentLuaMod(lua_State* state) {
  return static_cast<ModHost::LoadedLuaMod*>(luaApi().touserdata(state, luaUpvalueIndex(1)));
}
constexpr std::uintmax_t kMaxModArchiveBytes = 256U * 1024U * 1024U;
constexpr std::uint64_t kMaxModEntryBytes = 64U * 1024U * 1024U;
constexpr std::uint64_t kMaxModExtractedBytes = 512U * 1024U * 1024U;
constexpr std::uint16_t kMaxModArchiveEntries = 4096;
struct ZipEntry {
  std::string name;
  std::uint16_t compressionMethod = 0;
  std::uint32_t compressedSize = 0;
  std::uint32_t uncompressedSize = 0;
  std::uint32_t localHeaderOffset = 0;
};
std::string trimCopy(std::string value) {
  while(!value.empty() && std::isspace(static_cast<unsigned char>(value.front()))) {
    value.erase(value.begin());
  }
  while(!value.empty() && std::isspace(static_cast<unsigned char>(value.back()))) {
    value.pop_back();
  }
  return value;
}
std::string toLowerCopy(std::string value) {
  for(char& ch : value) {
    ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
  }
  return value;
}
std::string snakeToCamel(std::string_view snake) {
  std::string out;
  out.reserve(snake.size());
  bool upperNext = false;
  for(char c : snake) {
    if(c == '_' || c == '-') {
      upperNext = true;
      continue;
    }
    if(upperNext) {
      out.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(c))));
      upperNext = false;
    } else {
      out.push_back(c);
    }
  }
  return out;
}
std::string normalizeRelativePath(std::string_view value) {
  std::string normalized(value);
  std::replace(normalized.begin(), normalized.end(), '\\', '/');
  while(!normalized.empty() && normalized.front() == '/') {
    normalized.erase(normalized.begin());
  }
  return normalized;
}
bool isSafeRelativePath(std::string_view value) {
  const std::string normalized = normalizeRelativePath(value);
  if(normalized.empty() || normalized.find(':') != std::string::npos) {
    return false;
  }
  std::stringstream stream(normalized);
  std::string part;
  while(std::getline(stream, part, '/')) {
    if(part.empty() || part == ".") {
      continue;
    }
    if(part == "..") {
      return false;
    }
  }
  return true;
}
bool isSafeModId(std::string_view value) {
  return !value.empty() && std::all_of(value.begin(), value.end(), [](unsigned char ch) {
    return std::isalnum(ch) || ch == '_' || ch == '-' || ch == '.';
  });
}
bool isDirectoryZipPath(std::string_view value) {
  const std::string normalized = normalizeRelativePath(value);
  return !normalized.empty() && normalized.back() == '/';
}
std::string sanitizeName(std::string_view value) {
  std::string out;
  out.reserve(value.size());
  for(char ch : value) {
    if(std::isalnum(static_cast<unsigned char>(ch))) {
      out.push_back(ch);
    } else {
      out.push_back('_');
    }
  }
  while(!out.empty() && out.back() == '_') {
    out.pop_back();
  }
  return out.empty() ? "mod" : out;
}
std::vector<std::uint8_t> readFileBytes(const std::filesystem::path& path) {
  std::ifstream input(path, std::ios::binary);
  if(!input.is_open()) {
    return {};
  }
  input.seekg(0, std::ios::end);
  const std::streamsize size = input.tellg();
  input.seekg(0, std::ios::beg);
  if(size <= 0) {
    return {};
  }
  std::vector<std::uint8_t> bytes(static_cast<std::size_t>(size));
  if(!input.read(reinterpret_cast<char*>(bytes.data()), size)) {
    return {};
  }
  return bytes;
}
std::string readFileText(const std::filesystem::path& path) {
  const std::vector<std::uint8_t> bytes = readFileBytes(path);
  return std::string(bytes.begin(), bytes.end());
}
bool writeFileBytes(const std::filesystem::path& path, const std::vector<std::uint8_t>& bytes) {
  std::filesystem::create_directories(path.parent_path());
  std::ofstream output(path, std::ios::binary | std::ios::trunc);
  if(!output.is_open()) {
    return false;
  }
  if(!bytes.empty()) {
    output.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
  }
  return output.good();
}
bool writeFileText(const std::filesystem::path& path, const std::string& text) {
  std::filesystem::create_directories(path.parent_path());
  std::ofstream output(path, std::ios::binary | std::ios::trunc);
  if(!output.is_open()) {
    return false;
  }
  output << text;
  return output.good();
}
std::uint16_t readU16(const std::vector<std::uint8_t>& data, std::size_t offset) {
  if(offset + 2 > data.size()) {
    return 0;
  }
  return static_cast<std::uint16_t>(data[offset]) | (static_cast<std::uint16_t>(data[offset + 1]) << 8U);
}
std::uint32_t readU32(const std::vector<std::uint8_t>& data, std::size_t offset) {
  if(offset + 4 > data.size()) {
    return 0;
  }
  return static_cast<std::uint32_t>(data[offset]) | (static_cast<std::uint32_t>(data[offset + 1]) << 8U) |
         (static_cast<std::uint32_t>(data[offset + 2]) << 16U) |
         (static_cast<std::uint32_t>(data[offset + 3]) << 24U);
}
bool buildZipIndex(const std::vector<std::uint8_t>& archive, std::vector<ZipEntry>& entries) {
  if(archive.size() < 22) {
    return false;
  }
  std::size_t eocdOffset = std::string::npos;
  for(std::size_t i = archive.size() - 22; i != std::string::npos; --i) {
    if(readU32(archive, i) == 0x06054b50U) {
      eocdOffset = i;
      break;
    }
    if(i == 0) {
      break;
    }
  }
  if(eocdOffset == std::string::npos) {
    return false;
  }
  const std::uint32_t centralDirOffset = readU32(archive, eocdOffset + 16);
  const std::uint16_t entryCount = readU16(archive, eocdOffset + 10);
  if(entryCount > kMaxModArchiveEntries) {
    return false;
  }
  std::uint64_t extractedBytes = 0;
  std::size_t offset = centralDirOffset;
  for(std::uint16_t i = 0; i < entryCount; ++i) {
    if(offset + 46 > archive.size() || readU32(archive, offset) != 0x02014b50U) {
      return false;
    }
    ZipEntry entry;
    entry.compressionMethod = readU16(archive, offset + 10);
    entry.compressedSize = readU32(archive, offset + 20);
    entry.uncompressedSize = readU32(archive, offset + 24);
    extractedBytes += entry.uncompressedSize;
    if(entry.uncompressedSize > kMaxModEntryBytes || extractedBytes > kMaxModExtractedBytes) {
      return false;
    }
    const std::uint16_t nameLength = readU16(archive, offset + 28);
    const std::uint16_t extraLength = readU16(archive, offset + 30);
    const std::uint16_t commentLength = readU16(archive, offset + 32);
    entry.localHeaderOffset = readU32(archive, offset + 42);
    if(offset + 46 + nameLength > archive.size()) {
      return false;
    }
    entry.name.assign(reinterpret_cast<const char*>(archive.data() + offset + 46), nameLength);
    entries.push_back(std::move(entry));
    offset += 46 + nameLength + extraLength + commentLength;
  }
  return true;
}
const ZipEntry* findZipEntry(const std::vector<ZipEntry>& entries, std::string_view path) {
  const std::string normalized = normalizeRelativePath(path);
  for(const ZipEntry& entry : entries) {
    if(normalizeRelativePath(entry.name) == normalized) {
      return &entry;
    }
  }
  return nullptr;
}
std::vector<std::uint8_t> readZipEntryData(const std::vector<std::uint8_t>& archive, const ZipEntry& entry) {
  const std::size_t offset = entry.localHeaderOffset;
  if(offset + 30 > archive.size() || readU32(archive, offset) != 0x04034b50U) {
    return {};
  }
  const std::uint16_t nameLength = readU16(archive, offset + 26);
  const std::uint16_t extraLength = readU16(archive, offset + 28);
  const std::size_t dataOffset = offset + 30 + nameLength + extraLength;
  if(dataOffset + entry.compressedSize > archive.size()) {
    return {};
  }
  std::vector<std::uint8_t> compressed(archive.begin() + static_cast<std::ptrdiff_t>(dataOffset),
                                       archive.begin() +
                                           static_cast<std::ptrdiff_t>(dataOffset + entry.compressedSize));
  if(entry.compressionMethod == 0) {
    return compressed;
  }
  if(entry.compressionMethod == 8) {
    if(std::vector<std::uint8_t> inflated = decompressRawDeflate(compressed, entry.uncompressedSize); !inflated.empty()) {
      return inflated;
    }
    try {
      return zlibDecompress(compressed);
    } catch(...) {
      return {};
    }
  }
  return {};
}
ModPackage makeBrokenPackage(ModPackageSource source, const std::filesystem::path& sourcePath, std::string id,
                             std::string error) {
  ModPackage info;
  info.id = std::move(id);
  info.name = sourcePath.filename().string();
  info.source = source;
  info.enabledByDefault = false;
  info.configuredEnabled = false;
  info.error = std::move(error);
  info.sourcePath = sourcePath;
  return info;
}
bool parseManifestJson(const std::string& manifestTextRaw, ModPackage& out, const std::filesystem::path& sourcePath,
                       ModPackageSource source, std::string errorPrefix) {
  std::string manifestText = manifestTextRaw;
  if(manifestText.size() >= 3 && static_cast<unsigned char>(manifestText[0]) == 0xEF &&
     static_cast<unsigned char>(manifestText[1]) == 0xBB && static_cast<unsigned char>(manifestText[2]) == 0xBF) {
    manifestText.erase(0, 3);
  }
  const std::optional<std::string> id = util::json::stringField(manifestText, "id");
  if(!id.has_value() || trimCopy(*id).empty()) {
    out = makeBrokenPackage(source, sourcePath, sanitizeName(sourcePath.stem().string()), errorPrefix + "missing id");
    return false;
  }
  out.id = trimCopy(*id);
  if(!isSafeModId(out.id)) {
    out = makeBrokenPackage(source, sourcePath, sanitizeName(out.id),
                            errorPrefix + "id may contain only letters, digits, '.', '_' and '-'");
    return false;
  }
  const std::optional<std::string> name = util::json::stringField(manifestText, "name");
  out.name = name.has_value() && !trimCopy(*name).empty() ? trimCopy(*name) : out.id;
  if(const std::optional<std::string> version = util::json::stringField(manifestText, "version")) {
    out.version = trimCopy(*version);
  }
  if(const std::optional<std::string> description = util::json::stringField(manifestText, "description")) {
    out.description = trimCopy(*description);
  }
  if(const std::optional<std::string> entry = util::json::stringField(manifestText, "entry")) {
    out.entry = normalizeRelativePath(trimCopy(*entry));
  }
  if(out.entry.empty()) {
    if(const std::optional<std::string> script = util::json::stringField(manifestText, "script")) {
      out.entry = normalizeRelativePath(trimCopy(*script));
    }
  }
  out.source = source;
  out.sourcePath = sourcePath;
  out.enabledByDefault = util::json::boolField(manifestText, "enabled").value_or(true);
  out.configuredEnabled = out.enabledByDefault;
  out.runtimeScript = !out.entry.empty() && toLowerCopy(out.entry).ends_with(".lua");
  if(!out.entry.empty() && !isSafeRelativePath(out.entry)) {
    out.error = "Unsafe Lua script path";
    out.enabledByDefault = false;
    out.configuredEnabled = false;
  } else if(!out.entry.empty() && !out.runtimeScript) {
    out.error = "Only Lua script entries are supported; use entry/script ending in .lua";
    out.enabledByDefault = false;
    out.configuredEnabled = false;
  }
  return true;
}
void sortMods(std::vector<ModPackage>& mods) {
  std::stable_sort(mods.begin(), mods.end(), [](const ModPackage& lhs, const ModPackage& rhs) {
    const std::string lhsName = toLowerCopy(lhs.name.empty() ? lhs.id : lhs.name);
    const std::string rhsName = toLowerCopy(rhs.name.empty() ? rhs.id : rhs.name);
    if(lhsName != rhsName) {
      return lhsName < rhsName;
    }
    return lhs.id < rhs.id;
  });
}
template <typename Fill, typename Apply>
void callLuaEvent(const std::shared_ptr<ModHost::LoadedLuaMod>& mod, int ref, Fill fill, Apply apply);
[[nodiscard]] bool luaWorldIsOverworld(const World* world) {
  return world != nullptr && world->dimension != nullptr && !world->dimension->isNether && !world->dimension->hasCeiling;
}
void setWorldContextFields(lua_State* state, const World* world) {
  setField(state, "has_world", world != nullptr);
  setField(state, "world_name", world != nullptr ? world->name() : std::string());
  setField(state, "is_overworld", luaWorldIsOverworld(world));
  setField(state, "mod_generation", world != nullptr && world->isLuaModGenerationEnabled());
}
void pushStringMap(lua_State* state, const std::unordered_map<std::string, std::string>& values) {
  LuaApi& api = luaApi();
  api.createtable(state, 0, static_cast<int>(values.size()));
  for(const auto& [key, value] : values) {
    api.pushlstring(state, value.data(), value.size());
    api.setfield(state, -2, key.c_str());
  }
}
void readStringMapField(lua_State* state, int tableIndex, const char* key,
                        std::unordered_map<std::string, std::string>& values) {
  LuaApi& api = luaApi();
  api.getfield(state, tableIndex, key);
  if(api.type(state, -1) != kLuaTTable) {
    pop(state, 1);
    return;
  }
  const int mapIndex = api.gettop(state);
  std::unordered_map<std::string, std::string> parsed;
  parsed.reserve(std::min<std::size_t>(api.rawlen(state, mapIndex), 256));
  api.pushnil(state);
  while(parsed.size() < 256 && api.next(state, mapIndex) != 0) {
    if(api.type(state, -2) == kLuaTString && api.type(state, -1) == kLuaTString) {
      const std::string mapKey = luaString(state, -2, "");
      const std::string mapValue = luaString(state, -1, "");
      if(!mapKey.empty() && mapKey.size() <= 128 && mapValue.size() <= 4096) {
        parsed[mapKey] = mapValue;
      }
    }
    pop(state, 1);
  }
  pop(state, 1);
  values = std::move(parsed);
}
void setClientTickFields(lua_State* state, const ClientTickEvent& event) {
  setField(state, "before", event.before);
  setField(state, "after_world", event.afterWorld);
  setField(state, "paused", event.paused);
  setField(state, "has_player", event.player != nullptr);
  setField(state, "has_world", event.world != nullptr);
  setWorldContextFields(state, event.world);
  double cameraY = 64.0;
  double playerY = 64.0;
  float playerFallDistance = 0.0f;
  bool playerOnGround = false;
#ifdef MINECRAFT_NATIVE_EXPORTS
  if(event.client != nullptr && event.client->camera != nullptr) {
    const entity::Entity* camera = event.client->camera;
    cameraY = camera->lastTickY + (camera->y - camera->lastTickY);
  }
  if(event.player != nullptr) {
    playerY = event.player->y;
    playerFallDistance = event.player->fallDistance;
    playerOnGround = event.player->onGround;
  }
#endif
  setField(state, "camera_y", cameraY);
  setField(state, "player_y", playerY);
  setField(state, "player_fall_distance", playerFallDistance);
  setField(state, "player_on_ground", playerOnGround);
  if(event.world != nullptr) {
    setField(state, "world_time", static_cast<double>(event.world->getTime() % 24000ULL));
    setField(state, "is_night", net::minecraft::mod::lua::worldIsNight(event.world));
  } else {
    setField(state, "world_time", 0.0);
    setField(state, "is_night", false);
  }
}
[[nodiscard]] std::optional<LifecyclePhase> lifecyclePhaseFromName(std::string_view rawName) {
  static const std::unordered_map<std::string, LifecyclePhase> kPhases = {
      {"bootstrap_starting", LifecyclePhase::BootstrapStarting},
      {"biome_registration", LifecyclePhase::BiomeRegistration},
      {"block_registration", LifecyclePhase::BlockRegistration},
      {"block_registry_finalize", LifecyclePhase::BlockRegistryFinalize},
      {"item_registration", LifecyclePhase::ItemRegistration},
      {"block_item_registration", LifecyclePhase::BlockItemRegistration},
      {"smelting_recipe_registration", LifecyclePhase::SmeltingRecipeRegistration},
      {"crafting_recipe_registration", LifecyclePhase::CraftingRecipeRegistration},
      {"entity_registration", LifecyclePhase::EntityRegistration},
      {"block_entity_registration", LifecyclePhase::BlockEntityRegistration},
      {"fuel_registration", LifecyclePhase::FuelRegistration},
      {"client_renderer_registration", LifecyclePhase::ClientRendererRegistration},
      {"particle_registration", LifecyclePhase::ParticleRegistration},
      {"frozen", LifecyclePhase::Frozen},
  };
  const auto it = kPhases.find(toLowerCopy(std::string(rawName)));
  if(it == kPhases.end()) {
    return std::nullopt;
  }
  return it->second;
}
using net::minecraft::mod::lua::blockIdFromName;
using net::minecraft::mod::lua::BlockRegistrationSpec;
using net::minecraft::mod::lua::ChunkWriteMode;
using net::minecraft::mod::lua::ConnectionRule;
using net::minecraft::mod::lua::countEntitiesByName;
using net::minecraft::mod::lua::getBlockIdAt;
using net::minecraft::mod::lua::LuaBlockModelSpec;
using net::minecraft::mod::lua::LuaChunkContext;
using net::minecraft::mod::lua::ModelBox;
using net::minecraft::mod::lua::normalizedCelestial;
using net::minecraft::mod::lua::readPlayerPosition;
using net::minecraft::mod::lua::registerBlockSpec;
using net::minecraft::mod::lua::setPlayerCursorItem;
using net::minecraft::mod::lua::spawnClientParticle;
using net::minecraft::mod::lua::spawnEntityByName;
using net::minecraft::mod::lua::worldIsNight;
using net::minecraft::mod::lua::worldRandomInt;
#ifdef MINECRAFT_NATIVE_EXPORTS
const client::option::OptionSpec* findOptionSpec(std::string_view rawKey) {
  if(rawKey.empty()) {
    return nullptr;
  }
  const std::string key(rawKey);
  if(const std::optional<const client::option::OptionSpec*> spec = client::option::OptionRegistry::byKey(key)) {
    return *spec;
  }
  const std::string camel = snakeToCamel(rawKey);
  if(camel != key) {
    if(const std::optional<const client::option::OptionSpec*> spec = client::option::OptionRegistry::byKey(camel)) {
      return *spec;
    }
  }
  return nullptr;
}
void pushSavedOptionValue(lua_State* state, const client::option::GameOptions& options,
                          const client::option::OptionSpec& spec) {
  LuaApi& api = luaApi();
  if(spec.save == nullptr) {
    api.pushnil(state);
    return;
  }
  std::ostringstream out;
  spec.save(options, out);
  const std::string text = out.str();
  if(text == "true" || text == "false") {
    api.pushboolean(state, text == "true" ? 1 : 0);
    return;
  }
  try {
    std::size_t pos = 0;
    const int intValue = std::stoi(text, &pos);
    if(pos == text.size()) {
      api.pushinteger(state, static_cast<long long>(intValue));
      return;
    }
  } catch(...) {
  }
  try {
    const double floatValue = std::stod(text);
    api.pushnumber(state, floatValue);
    return;
  } catch(...) {
  }
  api.pushstring(state, text.c_str());
}
void pushOptionSpecValue(lua_State* state, const client::option::GameOptions& options,
                         const client::option::OptionSpec& spec) {
  LuaApi& api = luaApi();
  switch(spec.kind) {
  case client::option::OptionSpec::Kind::Toggle:
    if(spec.getBool != nullptr) {
      api.pushboolean(state, spec.getBool(options) ? 1 : 0);
    } else {
      pushSavedOptionValue(state, options, spec);
    }
    break;
  case client::option::OptionSpec::Kind::Slider:
    if(spec.getFloat != nullptr) {
      api.pushnumber(state, static_cast<double>(spec.getFloat(options)));
    } else {
      pushSavedOptionValue(state, options, spec);
    }
    break;
  case client::option::OptionSpec::Kind::Cycle:
  case client::option::OptionSpec::Kind::Hidden:
  default:
    pushSavedOptionValue(state, options, spec);
    break;
  }
}
bool tryPushKeybindOption(lua_State* state, const client::option::GameOptions& options, std::string_view rawKey) {
  std::string key(rawKey);
  if(key.rfind("key_", 0) == 0) {
    key = "key." + key.substr(4);
  }
  const std::string lower = toLowerCopy(key);
  LuaApi& api = luaApi();
  for(int i = 0; i < client::option::GameOptions::kKeybindCount; ++i) {
    const client::option::KeyBinding* binding = options.allKeys[i];
    if(binding == nullptr) {
      continue;
    }
    const std::string translation = binding->translationKey;
    const std::string shortName =
        translation.rfind("key.", 0) == 0 ? translation.substr(4) : translation;
    if(lower == toLowerCopy(translation) || lower == toLowerCopy(shortName)) {
      api.pushinteger(state, static_cast<long long>(binding->code));
      return true;
    }
  }
  return false;
}
bool tryPushExtraOption(lua_State* state, const client::option::GameOptions& options, std::string_view rawKey) {
  const std::string lower = toLowerCopy(std::string(rawKey));
  LuaApi& api = luaApi();
  if(lower == "skin") {
    api.pushstring(state, options.skin.c_str());
    return true;
  }
  if(lower == "lastserver") {
    api.pushstring(state, options.lastServer.c_str());
    return true;
  }
  if(lower == "fancygraphics") {
    api.pushboolean(state, options.fancyGraphics ? 1 : 0);
    return true;
  }
  if(lower == "thirdperson") {
    api.pushboolean(state, options.thirdPerson ? 1 : 0);
    return true;
  }
  if(lower == "hidehud") {
    api.pushboolean(state, options.hideHud ? 1 : 0);
    return true;
  }
  if(lower == "renderclouds" || lower == "clouds_enabled") {
    api.pushboolean(state, (options.clouds & 3) != 2 ? 1 : 0);
    return true;
  }
  return tryPushKeybindOption(state, options, rawKey);
}
using net::minecraft::mod::texture;
namespace {
thread_local int g_luaGuiDepth = 0;
class ScopedLuaGuiDraw {
public:
  explicit ScopedLuaGuiDraw(bool enabled = true) : enabled_(enabled) {
    if(enabled_) {
      ++g_luaGuiDepth;
    }
  }
  ~ScopedLuaGuiDraw() {
    if(enabled_) {
      --g_luaGuiDepth;
    }
  }
  ScopedLuaGuiDraw(const ScopedLuaGuiDraw&) = delete;
  ScopedLuaGuiDraw& operator=(const ScopedLuaGuiDraw&) = delete;

private:
  bool enabled_ = false;
};
[[nodiscard]] bool luaGuiDrawActive() noexcept {
  return g_luaGuiDepth > 0;
}
client::render::item::ItemRenderer g_luaItemRenderer;
struct ActiveScreenUi {
  ScreenUiContext* context = nullptr;
  ModHost::LoadedLuaMod* mod = nullptr;
  int stackedY = 0;
  bool trackStacked = false;
};
ActiveScreenUi* activeScreenUi() {
  thread_local ActiveScreenUi session;
  return &session;
}
// ---- Lua-driven custom screen ------------------------------------------------
// A Screen subclass whose entire layout and logic live in a Lua mod. It owns
// native text fields and buttons (declared by Lua at init), so caret/focus and
// click handling are native, while drawing and behavior use one phased event.
class LuaScreen;
struct ActiveLuaScreen {
  LuaScreen* screen = nullptr;
  ModHost::LoadedLuaMod* mod = nullptr;
  bool initPhase = false;
};
ActiveLuaScreen* activeLuaScreen() {
  thread_local ActiveLuaScreen session;
  return &session;
}
enum class LuaScreenPhase {
  Init,
  Render,
  Tick,
  Key,
  Mouse,
  Scroll,
  Close,
};
struct LuaScreenEvent {
  LuaScreen* screen = nullptr;
  LuaScreenPhase phase = LuaScreenPhase::Init;
  int mouseX = 0;
  int mouseY = 0;
  float tickDelta = 0.0f;
  char character = 0;
  int keyCode = 0;
  int button = 0;
  bool released = false;
  int scrollDelta = 0;
  bool handled = false;
};
struct LuaScreenField {
  std::string name;
  std::unique_ptr<client::gui::widget::TextFieldWidget> widget;
  bool numeric = false;
  bool allowSign = false;
  bool allowDot = false;
};
class LuaScreen : public client::gui::screen::Screen {
public:
  explicit LuaScreen(std::string id) : id_(std::move(id)) {}
  [[nodiscard]] std::string_view getScreenUiId() const override {
    return id_;
  }
  [[nodiscard]] const std::string& id() const {
    return id_;
  }
  [[nodiscard]] bool shouldPause() const override {
    return true;
  }
  void init() override {
    enableTextInput();
    fields_.clear();
    ActiveLuaScreen* session = activeLuaScreen();
    session->screen = this;
    session->initPhase = true;
    LuaScreenEvent event;
    event.screen = this;
    event.phase = LuaScreenPhase::Init;
    hooks().publish(event);
    session->initPhase = false;
    session->screen = nullptr;
  }
  void tick() override {
    for(LuaScreenField& field : fields_) {
      if(field.widget != nullptr) {
        field.widget->tick();
      }
    }
    ActiveLuaScreen* session = activeLuaScreen();
    session->screen = this;
    LuaScreenEvent event;
    event.screen = this;
    event.phase = LuaScreenPhase::Tick;
    hooks().publish(event);
    session->screen = nullptr;
  }
  void render(int mouseX, int mouseY, float tickDelta) override {
    renderBackground();
    ActiveLuaScreen* session = activeLuaScreen();
    session->screen = this;
    LuaScreenEvent event;
    event.screen = this;
    event.phase = LuaScreenPhase::Render;
    event.mouseX = mouseX;
    event.mouseY = mouseY;
    event.tickDelta = tickDelta;
    {
      const ScopedLuaGuiDraw drawScope;
      hooks().publish(event);
    }
    session->screen = nullptr;
    if(fieldsVisible_) {
      for(LuaScreenField& field : fields_) {
        if(field.widget != nullptr) {
          field.widget->render();
        }
      }
    }
    client::gui::screen::Screen::render(mouseX, mouseY, tickDelta);
  }
  void mouseClicked(int mouseX, int mouseY, int button) override {
    if(fieldsVisible_) {
      for(LuaScreenField& field : fields_) {
        if(field.widget != nullptr) {
          field.widget->mouseClicked(mouseX, mouseY, button);
        }
      }
    }
    client::gui::screen::Screen::mouseClicked(mouseX, mouseY, button);
    ActiveLuaScreen* session = activeLuaScreen();
    session->screen = this;
    LuaScreenEvent event;
    event.screen = this;
    event.phase = LuaScreenPhase::Mouse;
    event.mouseX = mouseX;
    event.mouseY = mouseY;
    event.button = button;
    hooks().publish(event);
    session->screen = nullptr;
  }
  void mouseReleased(int mouseX, int mouseY, int button) override {
    client::gui::screen::Screen::mouseReleased(mouseX, mouseY, button);
    ActiveLuaScreen* session = activeLuaScreen();
    session->screen = this;
    LuaScreenEvent event;
    event.screen = this;
    event.phase = LuaScreenPhase::Mouse;
    event.mouseX = mouseX;
    event.mouseY = mouseY;
    event.button = button;
    event.released = true;
    hooks().publish(event);
    session->screen = nullptr;
  }
  void keyPressed(char character, int keyCode) override {
    if(fieldsVisible_) {
      for(LuaScreenField& field : fields_) {
        if(field.widget != nullptr && field.widget->focused) {
          if(!(character >= 32 && field.numeric && !charAllowed(field, character))) {
            field.widget->keyPressed(character, keyCode);
          }
          break;
        }
      }
    }
    ActiveLuaScreen* session = activeLuaScreen();
    session->screen = this;
    LuaScreenEvent event;
    event.screen = this;
    event.phase = LuaScreenPhase::Key;
    event.character = character;
    event.keyCode = keyCode;
    hooks().publish(event);
    session->screen = nullptr;
  }
  void mouseScrolled(int mouseX, int mouseY, int delta) override {
    ActiveLuaScreen* session = activeLuaScreen();
    session->screen = this;
    LuaScreenEvent event;
    event.screen = this;
    event.phase = LuaScreenPhase::Scroll;
    event.mouseX = mouseX;
    event.mouseY = mouseY;
    event.scrollDelta = delta;
    hooks().publish(event);
    session->screen = nullptr;
  }
  void removed() override {
    ActiveLuaScreen* session = activeLuaScreen();
    session->screen = this;
    LuaScreenEvent event;
    event.screen = this;
    event.phase = LuaScreenPhase::Close;
    hooks().publish(event);
    session->screen = nullptr;
  }
  client::gui::widget::TextFieldWidget* addField(const std::string& name, int x, int y, int width, int height,
                                                 const std::string& text, int maxLength, bool numeric, bool allowSign,
                                                 bool allowDot) {
    LuaScreenField field;
    field.name = name;
    field.numeric = numeric;
    field.allowSign = allowSign;
    field.allowDot = allowDot;
    field.widget =
        std::make_unique<client::gui::widget::TextFieldWidget>(this, textRenderer(), x, y, width, height, text);
    if(maxLength > 0) {
      field.widget->setMaxLength(maxLength);
    }
    client::gui::widget::TextFieldWidget* ptr = field.widget.get();
    fields_.push_back(std::move(field));
    return ptr;
  }
  [[nodiscard]] client::gui::widget::TextFieldWidget* field(const std::string& name) {
    for(LuaScreenField& entry : fields_) {
      if(entry.name == name) {
        return entry.widget.get();
      }
    }
    return nullptr;
  }
  void addLuaButton(int x, int y, int width, int height, std::string text, std::function<void()> onClick) {
    (void)addButton<client::gui::widget::ActionButtonWidget>(client::gui::widget::ActionButtonWidget::kNoId, x, y,
                                                             width, height, std::move(text), std::move(onClick));
  }
  void setFieldsVisible(bool visible) {
    fieldsVisible_ = visible;
  }

private:
  static bool charAllowed(const LuaScreenField& field, char character) {
    if(character >= '0' && character <= '9') {
      return true;
    }
    if(character == '-' && field.allowSign && field.widget->getText().empty()) {
      return true;
    }
    if(character == '.' && field.allowDot && field.widget->getText().find('.') == std::string::npos) {
      return true;
    }
    return false;
  }
  std::string id_;
  std::vector<LuaScreenField> fields_;
  bool fieldsVisible_ = true;
};
int retainButtonCallback(lua_State* state, ModHost::LoadedLuaMod* mod, int fnIndex) {
  if(mod == nullptr) {
    return kLuaNoRef;
  }
  LuaApi& api = luaApi();
  if(api.type(state, fnIndex) != kLuaTFunction) {
    return kLuaNoRef;
  }
  api.pushvalue(state, fnIndex);
  const int ref = api.ref(state, kLuaRegistryIndex);
  if(ref != kLuaNoRef) {
    mod->buttonCallbackRefs.push_back(ref);
  }
  return ref;
}
void invokeButtonCallback(ModHost::LoadedLuaMod* mod, int ref) {
  LuaApi& api = luaApi();
  if(mod == nullptr || ref == kLuaNoRef || !api.ready()) {
    return;
  }
  const std::lock_guard<std::recursive_mutex> lock(mod->stateMutex);
  if(!mod->active || mod->state == nullptr) {
    return;
  }
  auto* state = static_cast<lua_State*>(mod->state);
  api.rawgeti(state, kLuaRegistryIndex, ref);
  if(api.type(state, -1) != kLuaTFunction) {
    api.settop(state, -2);
    return;
  }
  const int status = api.pcallk(state, 0, 0, 0, 0, nullptr);
  if(status != kLuaOk) {
    const char* error = api.tolstring(state, -1, nullptr);
    runtimeLog(mod->modId, "error", error != nullptr ? error : "button callback failed");
    api.settop(state, -2);
  }
}
[[nodiscard]] std::uint32_t luaArgb(lua_State* state, int index, std::uint32_t fallback = 0xFFFFFFFFU) {
  LuaApi& api = luaApi();
  int isNumber = 0;
  const double value = api.tonumberx(state, index, &isNumber);
  if(isNumber == 0 || !std::isfinite(value)) {
    return fallback;
  }
  if(value < 0.0) {
    return static_cast<std::uint32_t>(static_cast<std::int64_t>(value));
  }
  return static_cast<std::uint32_t>(static_cast<std::uint64_t>(value));
}
void drawGuiFillRect(int x, int y, int width, int height, std::uint32_t color) {
  const int rgb = static_cast<int>(color & 0x00FFFFFFU);
  int alpha = static_cast<int>((color >> 24U) & 0xFFU);
  if(alpha == 0) {
    alpha = 255;
  }
  client::render::Tessellator& tess = client::render::Tessellator::INSTANCE;
  gl::GL11::glDisable(gl::GL11::GL_LIGHTING);
  gl::GL11::glDisable(gl::GL11::GL_DEPTH_TEST);
  gl::GL11::glEnable(gl::GL11::GL_BLEND);
  gl::GL11::glBlendFunc(gl::GL11::GL_SRC_ALPHA, gl::GL11::GL_ONE_MINUS_SRC_ALPHA);
  gl::GL11::glDisable(gl::GL11::GL_TEXTURE_2D);
  client::gui::draw::coloredQuad(tess, x, y, x + width, y + height, rgb, alpha, 0.0f);
}
} // namespace
#endif
bool readVec3Field(lua_State* state, int tableIndex, const char* key, float& x, float& y, float& z) {
  LuaApi& api = luaApi();
  api.getfield(state, tableIndex, key);
  if(api.type(state, -1) != kLuaTTable) {
    api.settop(state, -2);
    return false;
  }
  const int vecTable = api.gettop(state);
  x = luaFloatField(state, vecTable, "x", luaFloatAt(state, vecTable, 1, x));
  y = luaFloatField(state, vecTable, "y", luaFloatAt(state, vecTable, 2, y));
  z = luaFloatField(state, vecTable, "z", luaFloatAt(state, vecTable, 3, z));
  api.settop(state, -2);
  return true;
}
bool readModelBox(lua_State* state, int tableIndex, ModelBox& box) {
  float minX = box.minX;
  float minY = box.minY;
  float minZ = box.minZ;
  float maxX = box.maxX;
  float maxY = box.maxY;
  float maxZ = box.maxZ;
  if(!readVec3Field(state, tableIndex, "min", minX, minY, minZ)) {
    return false;
  }
  if(!readVec3Field(state, tableIndex, "max", maxX, maxY, maxZ)) {
    return false;
  }
  if(!std::isfinite(minX) || !std::isfinite(minY) || !std::isfinite(minZ) || !std::isfinite(maxX) ||
     !std::isfinite(maxY) || !std::isfinite(maxZ) || minX < 0.0f || minY < 0.0f || minZ < 0.0f ||
     maxX > 1.0f || maxY > 1.0f || maxZ > 1.0f || minX >= maxX || minY >= maxY || minZ >= maxZ) {
    return false;
  }
  box.minX = minX;
  box.minY = minY;
  box.minZ = minZ;
  box.maxX = maxX;
  box.maxY = maxY;
  box.maxZ = maxZ;
  return true;
}
[[nodiscard]] ConnectionRule connectionRuleFromName(std::string name) {
  name = toLowerCopy(std::move(name));
  if(name == "same") {
    return ConnectionRule::Same;
  }
  if(name == "opaque") {
    return ConnectionRule::Opaque;
  }
  if(name == "glass") {
    return ConnectionRule::Glass;
  }
  if(name == "fence") {
    return ConnectionRule::Fence;
  }
  return ConnectionRule::Opaque;
}
bool parseBlockModel(lua_State* state, int tableIndex, LuaBlockModelSpec& model, std::string& error) {
  LuaApi& api = luaApi();
  const std::string type = toLowerCopy(luaStringField(state, tableIndex, "type", "full_cube"));
  model.opaque = luaBoolField(state, tableIndex, "opaque", model.opaque);
  model.fullCube = luaBoolField(state, tableIndex, "full_cube", model.fullCube);
  model.collisionHeight = luaFloatField(state, tableIndex, "collision_height", model.collisionHeight);
  model.stackOnSame = luaBoolField(state, tableIndex, "stack_on_same", model.stackOnSame);
  model.requiresSolidBelow = luaBoolField(state, tableIndex, "requires_solid_below", model.requiresSolidBelow);
  model.coordinateBounds =
      luaBoolField(state, tableIndex, "coordinate_bounds", luaBoolField(state, tableIndex, "varied_bounds", model.coordinateBounds));
  model.coordinateColor =
      luaBoolField(state, tableIndex, "coordinate_color", luaBoolField(state, tableIndex, "coord_color", model.coordinateColor));
  model.boundsPadding = luaFloatField(state, tableIndex, "bounds_padding", model.boundsPadding);
  model.boundsOffset = luaFloatField(state, tableIndex, "bounds_offset", model.boundsOffset);
  model.minScale = luaFloatField(state, tableIndex, "min_scale", model.minScale);
  model.maxScale = luaFloatField(state, tableIndex, "max_scale", model.maxScale);
  const std::string colorMode = toLowerCopy(luaStringField(state, tableIndex, "color", ""));
  if(colorMode == "coordinate" || colorMode == "coords" || colorMode == "position") {
    model.coordinateColor = true;
  }
  if(type == "full_cube" || type == "simple" || type.empty()) {
    model.type = LuaBlockModelSpec::Type::FullCube;
    return true;
  }
  if(type == "box_list" || type == "connected_bars") {
    model.type = type == "connected_bars" ? LuaBlockModelSpec::Type::ConnectedBars : LuaBlockModelSpec::Type::BoxList;
    model.opaque = luaBoolField(state, tableIndex, "opaque", type == "connected_bars" ? false : model.opaque);
    model.fullCube = luaBoolField(state, tableIndex, "full_cube", false);
    api.getfield(state, tableIndex, "connect");
    if(api.type(state, -1) == kLuaTTable) {
      const int connectTable = api.gettop(state);
      for(int i = 1; i <= 8; ++i) {
        api.rawgeti(state, connectTable, i);
        if(api.type(state, -1) == kLuaTString) {
          model.connectRules.push_back(connectionRuleFromName(luaString(state, -1, "")));
        }
        api.settop(state, -2);
      }
    }
    api.settop(state, tableIndex);
    if(type == "connected_bars") {
      ModelBox core;
      core.alwaysDraw = true;
      api.getfield(state, tableIndex, "core");
      if(api.type(state, -1) != kLuaTTable || !readModelBox(state, api.gettop(state), core)) {
        api.settop(state, tableIndex);
        error = "connected_bars model requires core box with min/max";
        return false;
      }
      api.settop(state, tableIndex);
      model.boxes.push_back(core);
      const struct {
        const char* key;
        int north;
        int south;
        int east;
        int west;
      } arms[] = {
          {"north", 1, 0, 0, 0},
          {"south", 0, 1, 0, 0},
          {"east", 0, 0, 1, 0},
          {"west", 0, 0, 0, 1},
      };
      for(const auto& arm : arms) {
        api.getfield(state, tableIndex, arm.key);
        if(api.type(state, -1) == kLuaTTable) {
          ModelBox box;
          if(readModelBox(state, api.gettop(state), box)) {
            box.alwaysDraw = false;
            box.connectNorth = arm.north;
            box.connectSouth = arm.south;
            box.connectEast = arm.east;
            box.connectWest = arm.west;
            model.boxes.push_back(box);
          }
        }
        api.settop(state, tableIndex);
      }
      if(model.connectRules.empty()) {
        model.connectRules = {ConnectionRule::Same, ConnectionRule::Opaque, ConnectionRule::Glass,
                              ConnectionRule::Fence};
      }
      return true;
    }
    api.getfield(state, tableIndex, "boxes");
    if(api.type(state, -1) != kLuaTTable) {
      api.settop(state, tableIndex);
      error = "box_list model requires boxes array";
      return false;
    }
    const int boxesTable = api.gettop(state);
    for(int i = 1; i <= 32; ++i) {
      api.rawgeti(state, boxesTable, i);
      if(api.type(state, -1) == kLuaTNil) {
        api.settop(state, -2);
        break;
      }
      if(api.type(state, -1) == kLuaTTable) {
        ModelBox box;
        box.alwaysDraw = luaBoolField(state, api.gettop(state), "always", true);
        if(readModelBox(state, api.gettop(state), box)) {
          model.boxes.push_back(box);
        }
      }
      api.settop(state, -2);
    }
    api.settop(state, tableIndex);
    if(model.boxes.empty()) {
      error = "box_list model requires at least one box";
      return false;
    }
    return true;
  }
  error = "unknown model type: " + type;
  return false;
}
[[maybe_unused]] [[nodiscard]] ChunkWriteMode chunkWriteModeForStage(world::gen::ChunkStage stage) {
  switch(stage) {
  case world::gen::ChunkStage::Terrain:
  case world::gen::ChunkStage::Surface:
  case world::gen::ChunkStage::Carver:
    return ChunkWriteMode::RawGeneration;
  case world::gen::ChunkStage::Features:
    return ChunkWriteMode::ChunkApi;
  }
  return ChunkWriteMode::ChunkApi;
}
[[nodiscard]] const char* chunkStageName(world::gen::ChunkStage stage) {
  switch(stage) {
  case world::gen::ChunkStage::Terrain:
    return "terrain";
  case world::gen::ChunkStage::Surface:
    return "surface";
  case world::gen::ChunkStage::Carver:
    return "carver";
  case world::gen::ChunkStage::Features:
    return "features";
  }
  return "unknown";
}
[[nodiscard]] const char* renderStageName(WorldRenderStage stage) {
  switch(stage) {
  case WorldRenderStage::Sky:
    return "sky";
  case WorldRenderStage::Stars:
    return "stars";
  case WorldRenderStage::Clouds:
    return "clouds";
  }
  return "unknown";
}
[[nodiscard]] const char* renderMomentName(RenderHookMoment moment) {
  return moment == RenderHookMoment::Before ? "before" : "after";
}
void setChunkContextFields(lua_State* state) {
#ifdef MINECRAFT_NATIVE_EXPORTS
  setField(state, "chunk_x", LuaChunkContext::activeChunkX());
  setField(state, "chunk_z", LuaChunkContext::activeChunkZ());
  setField(state, "has_chunk", LuaChunkContext::hasActiveChunk());
#else
  setField(state, "chunk_x", 0);
  setField(state, "chunk_z", 0);
  setField(state, "has_chunk", false);
#endif
}
int luaLog(lua_State* state) {
  LuaApi& api = luaApi();
  ModHost::LoadedLuaMod* mod = currentLuaMod(state);
  const int top = api.gettop(state);
  const std::string level = top >= 2 ? luaString(state, 1, "info") : "info";
  const std::string message = top >= 2 ? luaString(state, 2, "") : luaString(state, 1, "");
  runtimeLog(mod != nullptr ? mod->modId : "", level.c_str(), message);
  return 0;
}
int luaTimeUtcMillis(lua_State* state) {
  const auto now = std::chrono::system_clock::now().time_since_epoch();
  luaApi().pushnumber(
      state, static_cast<double>(std::chrono::duration_cast<std::chrono::milliseconds>(now).count()));
  return 1;
}
int luaIsKeyDown(lua_State* state) {
  LuaApi& api = luaApi();
  if(api.gettop(state) < 1) {
    api.pushboolean(state, 0);
    return 1;
  }
  int isNumber = 0;
  const int key = static_cast<int>(api.tointegerx(state, 1, &isNumber));
  if(isNumber == 0) {
    api.pushboolean(state, 0);
    return 1;
  }
#ifdef MINECRAFT_NATIVE_EXPORTS
  api.pushboolean(state, client::input::InputSystem::instance().isKeyDown(key) ? 1 : 0);
#else
  (void)key;
  api.pushboolean(state, 0);
#endif
  return 1;
}
int defaultKeyCodeForName(const std::string& name) {
  static const std::unordered_map<std::string, int> kNamedCodes = {
      {"escape", 1},
      {"1", 2},
      {"2", 3},
      {"3", 4},
      {"4", 5},
      {"5", 6},
      {"6", 7},
      {"7", 8},
      {"8", 9},
      {"9", 10},
      {"0", 11},
      {"q", 16},
      {"w", 17},
      {"e", 18},
      {"r", 19},
      {"t", 20},
      {"y", 21},
      {"u", 22},
      {"i", 23},
      {"o", 24},
      {"p", 25},
      {"enter", 28},
      {"a", 30},
      {"s", 31},
      {"d", 32},
      {"f", 33},
      {"g", 34},
      {"h", 35},
      {"j", 36},
      {"k", 37},
      {"l", 38},
      {"z", 44},
      {"x", 45},
      {"c", 46},
      {"v", 47},
      {"b", 48},
      {"n", 49},
      {"m", 50},
      {"space", 57},
      {"up", 200},
      {"left_arrow", 203},
      {"right_arrow", 205},
      {"down", 208},
  };
  if(const auto it = kNamedCodes.find(name); it != kNamedCodes.end()) {
    return it->second;
  }
  return 0;
}
#ifdef MINECRAFT_NATIVE_EXPORTS
int keyCodeFromBindingName(const std::string& name, const client::option::GameOptions& options) {
  if(name == "forward" || name == "move_forward") {
    return options.forwardKey.code;
  }
  if(name == "left" || name == "move_left") {
    return options.leftKey.code;
  }
  if(name == "back" || name == "backward" || name == "move_back") {
    return options.backKey.code;
  }
  if(name == "right" || name == "move_right") {
    return options.rightKey.code;
  }
  if(name == "jump") {
    return options.jumpKey.code;
  }
  if(name == "sneak") {
    return options.sneakKey.code;
  }
  if(name == "drop") {
    return options.dropKey.code;
  }
  if(name == "inventory") {
    return options.inventoryKey.code;
  }
  if(name == "chat") {
    return options.chatKey.code;
  }
  if(name == "fog") {
    return options.fogKey.code;
  }
  return 0;
}
#endif
int luaKeyCode(lua_State* state) {
  LuaApi& api = luaApi();
  if(api.gettop(state) < 1) {
    api.pushinteger(state, 0);
    return 1;
  }
  if(api.type(state, 1) == kLuaTNumber) {
    int isNumber = 0;
    const int key = static_cast<int>(api.tointegerx(state, 1, &isNumber));
    api.pushinteger(state, isNumber != 0 ? key : 0);
    return 1;
  }
  std::string name = toLowerCopy(luaString(state, 1, ""));
#ifdef MINECRAFT_NATIVE_EXPORTS
  if(client::Minecraft* minecraft = client::Minecraft::INSTANCE; minecraft != nullptr) {
    if(const int bound = keyCodeFromBindingName(name, minecraft->options); bound != 0) {
      api.pushinteger(state, bound);
      return 1;
    }
  }
#endif
  api.pushinteger(state, defaultKeyCodeForName(name));
  return 1;
}
int luaAssetPath(lua_State* state) {
  LuaApi& api = luaApi();
  ModHost::LoadedLuaMod* mod = currentLuaMod(state);
  if(mod == nullptr || api.gettop(state) < 1) {
    api.pushnil(state);
    return 1;
  }
  const std::string relativePath = luaString(state, 1, "");
  const std::filesystem::path path = host().assetPath(mod->modId, relativePath);
  if(path.empty()) {
    api.pushnil(state);
    return 1;
  }
  const std::string text = path.string();
  api.pushstring(state, text.c_str());
  return 1;
}
int luaReadAsset(lua_State* state) {
  LuaApi& api = luaApi();
  ModHost::LoadedLuaMod* mod = currentLuaMod(state);
  if(mod == nullptr || api.gettop(state) < 1) {
    api.pushnil(state);
    return 1;
  }
  const std::string relativePath = luaString(state, 1, "");
  const std::filesystem::path path = host().assetPath(mod->modId, relativePath);
  if(path.empty() || !std::filesystem::is_regular_file(path)) {
    api.pushnil(state);
    return 1;
  }
  std::ifstream input(path, std::ios::binary);
  if(!input.is_open()) {
    api.pushnil(state);
    return 1;
  }
  std::ostringstream buffer;
  buffer << input.rdbuf();
  const std::string content = buffer.str();
  api.pushstring(state, content.c_str());
  return 1;
}
int luaReadGameFile(lua_State* state) {
  LuaApi& api = luaApi();
  ModHost::LoadedLuaMod* mod = currentLuaMod(state);
  if(mod == nullptr || api.gettop(state) < 1 || api.type(state, 1) != kLuaTString) {
    api.pushnil(state);
    return 1;
  }
  const std::string relativePath = luaString(state, 1, "");
  if(relativePath.empty() || !isSafeRelativePath(relativePath)) {
    api.pushnil(state);
    return 1;
  }
  const std::filesystem::path path = host().runDirectory() / "config" / "mods" / sanitizeName(mod->modId) /
                                     std::filesystem::path(normalizeRelativePath(relativePath));
  if(!std::filesystem::is_regular_file(path)) {
    const std::string normalized = normalizeRelativePath(relativePath);
    const std::string extension = toLowerCopy(std::filesystem::path(normalized).extension().string());
    if(normalized.find('/') != std::string::npos || (extension != ".cfg" && extension != ".txt")) {
      api.pushnil(state);
      return 1;
    }
    const std::filesystem::path legacyPath = host().runDirectory() / normalized;
    if(!std::filesystem::is_regular_file(legacyPath)) {
      api.pushnil(state);
      return 1;
    }
    const std::string legacy = readFileText(legacyPath);
    api.pushlstring(state, legacy.data(), legacy.size());
    return 1;
  }
  const std::string content = readFileText(path);
  api.pushlstring(state, content.data(), content.size());
  return 1;
}
int luaWriteGameFile(lua_State* state) {
  LuaApi& api = luaApi();
  ModHost::LoadedLuaMod* mod = currentLuaMod(state);
  if(mod == nullptr || api.gettop(state) < 2 || api.type(state, 1) != kLuaTString) {
    api.pushboolean(state, 0);
    return 1;
  }
  const std::string relativePath = luaString(state, 1, "");
  const std::string content = luaString(state, 2, "");
  if(relativePath.empty() || !isSafeRelativePath(relativePath)) {
    api.pushboolean(state, 0);
    return 1;
  }
  const std::filesystem::path path = host().runDirectory() / "config" / "mods" / sanitizeName(mod->modId) /
                                     std::filesystem::path(normalizeRelativePath(relativePath));
  api.pushboolean(state, writeFileText(path, content) ? 1 : 0);
  return 1;
}
#ifdef MINECRAFT_NATIVE_EXPORTS
int luaOptionsGet(lua_State* state) {
  LuaApi& api = luaApi();
  if(client::Minecraft::INSTANCE == nullptr || api.type(state, 1) != kLuaTString) {
    api.pushnil(state);
    return 1;
  }
  const std::string_view rawKey = luaString(state, 1, "");
  const client::option::GameOptions& options = client::Minecraft::INSTANCE->options;
  if(const client::option::OptionSpec* spec = findOptionSpec(rawKey)) {
    pushOptionSpecValue(state, options, *spec);
    return 1;
  }
  if(tryPushExtraOption(state, options, rawKey)) {
    return 1;
  }
  api.pushnil(state);
  return 1;
}
int luaOptionsKeys(lua_State* state) {
  LuaApi& api = luaApi();
  const std::span<const client::option::OptionSpec> specs = client::option::OptionRegistry::all();
  api.createtable(state, static_cast<int>(specs.size()), 0);
  for(std::size_t i = 0; i < specs.size(); ++i) {
    api.pushstring(state, std::string(specs[i].persistKey).c_str());
    api.rawseti(state, -2, static_cast<long long>(i + 1));
  }
  return 1;
}
#endif
int luaReadAssetBytes(lua_State* state) {
  LuaApi& api = luaApi();
  ModHost::LoadedLuaMod* mod = currentLuaMod(state);
  if(mod == nullptr || api.gettop(state) < 1 || api.type(state, 1) != kLuaTString) {
    api.pushnil(state);
    return 1;
  }
  const std::string relativePath = luaString(state, 1, "");
  bool gzip = false;
  if(api.gettop(state) >= 2 && api.type(state, 2) == kLuaTTable) {
    gzip = luaBoolField(state, 2, "gzip", false);
  }
  const std::filesystem::path path = host().assetPath(mod->modId, relativePath);
  if(path.empty() || !std::filesystem::is_regular_file(path)) {
    api.pushnil(state);
    return 1;
  }
  std::ifstream input(path, std::ios::binary);
  if(!input.is_open()) {
    api.pushnil(state);
    return 1;
  }
  std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
  if(bytes.empty()) {
    api.pushnil(state);
    return 1;
  }
  if(gzip) {
    try {
      bytes = gzipDecompress(bytes);
    } catch(const std::exception&) {
      api.pushnil(state);
      return 1;
    }
  }
  if(bytes.empty()) {
    api.pushnil(state);
    return 1;
  }
  api.pushlstring(state, reinterpret_cast<const char*>(bytes.data()), bytes.size());
  return 1;
}
int luaReadNbtAsset(lua_State* state) {
  ModHost::LoadedLuaMod* mod = currentLuaMod(state);
  const std::string relativePath = normalizeRelativePath(luaString(state, 1, ""));
  const std::filesystem::path path = mod != nullptr ? host().assetPath(mod->modId, relativePath)
                                                    : std::filesystem::path{};
  if(path.empty() || !std::filesystem::is_regular_file(path) ||
     std::filesystem::file_size(path) > kMaxModEntryBytes) {
    luaApi().pushnil(state);
    luaApi().pushstring(state, "NBT asset not found or too large");
    return 2;
  }
  try {
    const std::vector<std::uint8_t> bytes = readFileBytes(path);
    const bool gzip = bytes.size() >= 2 && bytes[0] == 0x1f && bytes[1] == 0x8b;
    const Nbt root = gzip ? Nbt::readCompressed(bytes) : Nbt::read(bytes);
    pushNbtValue(state, root);
    luaApi().pushnil(state);
  } catch(const std::exception& error) {
    luaApi().pushnil(state);
    luaApi().pushstring(state, error.what());
  }
  return 2;
}
[[nodiscard]] bool isSupportedLuaEvent(std::string_view event) {
  static const std::set<std::string_view> kEvents = {
      "attack_damage",
      "block_interact",
      "chunk_generation",
      "client_tick",
      "create_world",
      "entity_interact",
      "fov",
      "key_press",
      "mouse_button",
      "player_travel",
      "screen_event",
      "screen_region",
      "screen_ui",
      "tick_rate",
      "world_color",
      "world_render",
      "world_spawn_search",
      "world_open",
      "world_start",
      "world_tick",
  };
  return kEvents.contains(event);
}
int luaOn(lua_State* state) {
  LuaApi& api = luaApi();
  ModHost::LoadedLuaMod* mod = currentLuaMod(state);
  if(mod == nullptr || api.gettop(state) < 2 || api.type(state, 1) != kLuaTString ||
     api.type(state, 2) != kLuaTFunction) {
    runtimeLog(mod != nullptr ? mod->modId : "", "error", "minecraft.on expects (event_name, function, priority?)");
    api.pushboolean(state, 0);
    return 1;
  }
  const std::string eventName = toLowerCopy(luaString(state, 1, ""));
  if(!isSupportedLuaEvent(eventName)) {
    runtimeLog(mod->modId, "error", "unknown Lua event: " + eventName);
    api.pushboolean(state, 0);
    return 1;
  }
  int priority = 0;
  if(api.gettop(state) >= 3) {
    int isNumber = 0;
    const long long value = api.tointegerx(state, 3, &isNumber);
    if(isNumber != 0) {
      priority = static_cast<int>(value);
    }
  }
  api.pushvalue(state, 2);
  const int ref = api.ref(state, kLuaRegistryIndex);
  if(ref == kLuaNoRef) {
    runtimeLog(mod->modId, "error", "failed to retain Lua callback");
    api.pushboolean(state, 0);
    return 1;
  }
  mod->callbacks.push_back({eventName, ref, priority});
  api.pushboolean(state, 1);
  return 1;
}
int luaAtPhase(lua_State* state) {
  LuaApi& api = luaApi();
  ModHost::LoadedLuaMod* mod = currentLuaMod(state);
  if(mod == nullptr || api.gettop(state) < 3 || api.type(state, 1) != kLuaTString ||
     api.type(state, 3) != kLuaTFunction) {
    runtimeLog(mod != nullptr ? mod->modId : "", "error", "minecraft.at_phase expects (phase_name, order, function)");
    return 0;
  }
  const std::optional<LifecyclePhase> phase = lifecyclePhaseFromName(luaString(state, 1, ""));
  if(!phase.has_value()) {
    runtimeLog(mod->modId, "error", "unknown lifecycle phase: " + luaString(state, 1, ""));
    return 0;
  }
  int order = 0;
  int isNumber = 0;
  const long long orderValue = api.tointegerx(state, 2, &isNumber);
  if(isNumber != 0) {
    order = static_cast<int>(orderValue);
  }
  api.pushvalue(state, 3);
  const int ref = api.ref(state, kLuaRegistryIndex);
  if(ref == kLuaNoRef) {
    runtimeLog(mod->modId, "error", "failed to retain Lua lifecycle callback");
    return 0;
  }
  const LifecyclePhase targetPhase = *phase;
  const std::string modId = mod->modId;
  hooks().subscribe<LifecycleEvent>(order, [modId, ref, targetPhase](LifecycleEvent& event) {
    if(event.current != targetPhase) {
      return;
    }
    for(const std::shared_ptr<ModHost::LoadedLuaMod>& loaded : host().loadedMods()) {
      if(loaded == nullptr || !loaded->active || loaded->modId != modId) {
        continue;
      }
      callLuaEvent(
          loaded, ref,
          [&event](lua_State* luaState) {
            setField(luaState, "previous", static_cast<int>(event.previous));
            setField(luaState, "current", static_cast<int>(event.current));
          },
          [](lua_State*) {});
      break;
    }
  });
  return 0;
}
int luaChunkSetBlock(lua_State* state) {
#ifdef MINECRAFT_NATIVE_EXPORTS
  LuaApi& api = luaApi();
  if(!LuaChunkContext::hasActiveChunk() || api.gettop(state) < 4) {
    return 0;
  }
  int isNumber = 0;
  const int localX = static_cast<int>(api.tointegerx(state, 1, &isNumber));
  const int y = static_cast<int>(api.tointegerx(state, 2, &isNumber));
  const int localZ = static_cast<int>(api.tointegerx(state, 3, &isNumber));
  const int blockId = static_cast<int>(api.tointegerx(state, 4, &isNumber));
  api.pushboolean(state, LuaChunkContext::setBlock(localX, y, localZ, blockId) ? 1 : 0);
  return 1;
#else
  (void)state;
  return 0;
#endif
}
int luaChunkFillBlock(lua_State* state) {
#ifdef MINECRAFT_NATIVE_EXPORTS
  LuaApi& api = luaApi();
  if(!LuaChunkContext::hasActiveChunk() || api.gettop(state) < 7) {
    api.pushinteger(state, 0);
    return 1;
  }
  int isNumber = 0;
  int x1 = static_cast<int>(api.tointegerx(state, 1, &isNumber));
  int y1 = static_cast<int>(api.tointegerx(state, 2, &isNumber));
  int z1 = static_cast<int>(api.tointegerx(state, 3, &isNumber));
  int x2 = static_cast<int>(api.tointegerx(state, 4, &isNumber));
  int y2 = static_cast<int>(api.tointegerx(state, 5, &isNumber));
  int z2 = static_cast<int>(api.tointegerx(state, 6, &isNumber));
  const int blockId = static_cast<int>(api.tointegerx(state, 7, &isNumber));
  if(x1 > x2) {
    std::swap(x1, x2);
  }
  if(y1 > y2) {
    std::swap(y1, y2);
  }
  if(z1 > z2) {
    std::swap(z1, z2);
  }
  x1 = std::clamp(x1, 0, Chunk::width - 1);
  x2 = std::clamp(x2, 0, Chunk::width - 1);
  y1 = std::clamp(y1, 0, Chunk::height - 1);
  y2 = std::clamp(y2, 0, Chunk::height - 1);
  z1 = std::clamp(z1, 0, Chunk::depth - 1);
  z2 = std::clamp(z2, 0, Chunk::depth - 1);
  int changed = 0;
  for(int y = y1; y <= y2; ++y) {
    for(int z = z1; z <= z2; ++z) {
      for(int x = x1; x <= x2; ++x) {
        if(LuaChunkContext::setBlock(x, y, z, blockId)) {
          ++changed;
        }
      }
    }
  }
  api.pushinteger(state, changed);
  return 1;
#else
  (void)state;
  return 0;
#endif
}
int luaChunkGetBlock(lua_State* state) {
#ifdef MINECRAFT_NATIVE_EXPORTS
  LuaApi& api = luaApi();
  if(!LuaChunkContext::hasActiveChunk() || api.gettop(state) < 3) {
    api.pushinteger(state, 0);
    return 1;
  }
  int isNumber = 0;
  const int localX = static_cast<int>(api.tointegerx(state, 1, &isNumber));
  const int y = static_cast<int>(api.tointegerx(state, 2, &isNumber));
  const int localZ = static_cast<int>(api.tointegerx(state, 3, &isNumber));
  api.pushinteger(state, LuaChunkContext::getBlock(localX, y, localZ));
  return 1;
#else
  (void)state;
  return 0;
#endif
}
int luaChunkGetHeight(lua_State* state) {
#ifdef MINECRAFT_NATIVE_EXPORTS
  LuaApi& api = luaApi();
  if(!LuaChunkContext::hasActiveChunk() || api.gettop(state) < 2) {
    api.pushinteger(state, 0);
    return 1;
  }
  int isNumber = 0;
  const int localX = static_cast<int>(api.tointegerx(state, 1, &isNumber));
  const int localZ = static_cast<int>(api.tointegerx(state, 2, &isNumber));
  api.pushinteger(state, LuaChunkContext::getHeight(localX, localZ));
  return 1;
#else
  (void)state;
  return 0;
#endif
}
int luaRegisterBlock(lua_State* state) {
  LuaApi& api = luaApi();
  ModHost::LoadedLuaMod* mod = currentLuaMod(state);
  if(mod == nullptr || api.gettop(state) < 1 || api.type(state, 1) != kLuaTTable) {
    api.pushboolean(state, 0);
    api.pushstring(state, "minecraft.register_block expects a spec table");
    return 2;
  }
  const int tableIndex = 1;
  BlockRegistrationSpec spec;
  spec.blockId = luaIntField(state, tableIndex, "id", 0);
  spec.texturePath = luaStringField(state, tableIndex, "texture", "");
  spec.terrainTextureId = luaIntField(state, tableIndex, "texture_id", -1);
  spec.hardness = luaFloatField(state, tableIndex, "hardness", 1.0f);
  spec.resistance = luaFloatField(state, tableIndex, "resistance", 1.0f);
  spec.luminance = luaFloatField(state, tableIndex, "luminance", 0.0f);
  spec.translationKey = luaStringField(state, tableIndex, "translation_key", "");
  spec.material = luaStringField(state, tableIndex, "material", "stone");
  api.getfield(state, tableIndex, "model");
  if(api.type(state, -1) == kLuaTTable) {
    std::string modelError;
    if(!parseBlockModel(state, api.gettop(state), spec.model, modelError)) {
      api.settop(state, tableIndex);
      api.pushboolean(state, 0);
      api.pushstring(state, modelError.c_str());
      return 2;
    }
  }
  api.settop(state, tableIndex);
  std::string error;
  if(!registerBlockSpec(spec, error)) {
    api.pushboolean(state, 0);
    api.pushstring(state, error.c_str());
    return 2;
  }
  WorldRequiredMods::registerContentBlock(mod->modId, spec.blockId);
  api.pushboolean(state, 1);
  return 1;
}
int luaRegisterShapedRecipe(lua_State* state) {
  LuaApi& api = luaApi();
  ModHost::LoadedLuaMod* mod = currentLuaMod(state);
  if(mod == nullptr || api.gettop(state) < 1 || api.type(state, 1) != kLuaTTable) {
    api.pushboolean(state, 0);
    api.pushstring(state, "minecraft.register_shaped_recipe expects a spec table");
    return 2;
  }
  const int tableIndex = 1;
  lua::ShapedRecipeSpec spec;
  spec.outputBlockId = luaIntField(state, tableIndex, "output_block_id", 0);
  spec.outputCount = luaIntField(state, tableIndex, "output_count", 1);
  spec.ingredientItemId = luaIntField(state, tableIndex, "item_id", 0);
  const std::string keyText = luaStringField(state, tableIndex, "key", "#");
  spec.key = keyText.empty() ? '#' : keyText.front();
  api.getfield(state, tableIndex, "pattern");
  if(api.type(state, -1) == kLuaTTable) {
    const int patternTable = api.gettop(state);
    for(int i = 1; i <= 8; ++i) {
      api.rawgeti(state, patternTable, i);
      if(api.type(state, -1) == kLuaTNil) {
        api.settop(state, -2);
        break;
      }
      if(api.type(state, -1) == kLuaTString) {
        spec.pattern.push_back(luaString(state, -1, ""));
      }
      api.settop(state, -2);
    }
  }
  api.settop(state, tableIndex);
  std::string error;
  if(!lua::registerShapedRecipe(spec, error)) {
    api.pushboolean(state, 0);
    api.pushstring(state, error.c_str());
    return 2;
  }
  api.pushboolean(state, 1);
  return 1;
}
#ifdef MINECRAFT_NATIVE_EXPORTS
int luaWorldBlockId(lua_State* state) {
  LuaApi& api = luaApi();
  if(api.type(state, 1) != kLuaTString) {
    api.pushinteger(state, 0);
    return 1;
  }
  api.pushinteger(state, blockIdFromName(luaString(state, 1, "").c_str()));
  return 1;
}
int luaWorldRandom(lua_State* state) {
  LuaApi& api = luaApi();
  int bound = 1000;
  if(api.gettop(state) >= 1 && api.type(state, 1) == kLuaTNumber) {
    int isNumber = 0;
    bound = static_cast<int>(api.tonumberx(state, 1, &isNumber));
  }
  World* world = LuaChunkContext::activeWorld();
  if(world == nullptr && client::Minecraft::INSTANCE != nullptr) {
    world = client::Minecraft::INSTANCE->world;
  }
  api.pushinteger(state, worldRandomInt(world, bound));
  return 1;
}
int luaWorldIsNight(lua_State* state) {
  (void)state;
  LuaApi& api = luaApi();
  World* world = LuaChunkContext::activeWorld();
  if(world == nullptr && client::Minecraft::INSTANCE != nullptr) {
    world = client::Minecraft::INSTANCE->world;
  }
  api.pushboolean(state, worldIsNight(world) ? 1 : 0);
  return 1;
}
int luaWorldTopSolidY(lua_State* state) {
  LuaApi& api = luaApi();
  int isNumber = 0;
  const int x = static_cast<int>(api.tonumberx(state, 1, &isNumber));
  const int z = static_cast<int>(api.tonumberx(state, 2, &isNumber));
  World* world = LuaChunkContext::activeWorld();
  if(world == nullptr && client::Minecraft::INSTANCE != nullptr) {
    world = client::Minecraft::INSTANCE->world;
  }
  if(world == nullptr) {
    api.pushinteger(state, -1);
    return 1;
  }
  api.pushinteger(state, world->getTopSolidBlockY(x, z));
  return 1;
}
int luaWorldPlayer(lua_State* state) {
  LuaApi& api = luaApi();
  double x = 0.0;
  double y = 0.0;
  double z = 0.0;
  if(!readPlayerPosition(x, y, z)) {
    api.pushnil(state);
    return 1;
  }
  api.createtable(state, 0, 3);
  setField(state, "x", x);
  setField(state, "y", y);
  setField(state, "z", z);
  return 1;
}
int luaWorldSpawnEntity(lua_State* state) {
  LuaApi& api = luaApi();
  if(api.gettop(state) < 2 || api.type(state, 1) != kLuaTString) {
    api.pushboolean(state, 0);
    return 1;
  }
  const std::string entityId = luaString(state, 1, "");
  double x = 0.0;
  double y = 0.0;
  double z = 0.0;
  if(api.type(state, 2) == kLuaTTable) {
    x = luaFloatField(state, 2, "x", luaFloatAt(state, 2, 1, 0.0f));
    y = luaFloatField(state, 2, "y", luaFloatAt(state, 2, 2, 64.0f));
    z = luaFloatField(state, 2, "z", luaFloatAt(state, 2, 3, 0.0f));
  } else {
    int isNumber = 0;
    x = api.tonumberx(state, 2, &isNumber);
    y = api.gettop(state) >= 3 ? api.tonumberx(state, 3, &isNumber) : 64.0;
    z = api.gettop(state) >= 4 ? api.tonumberx(state, 4, &isNumber) : 0.0;
  }
  World* world = LuaChunkContext::activeWorld();
  if(world == nullptr && client::Minecraft::INSTANCE != nullptr) {
    world = client::Minecraft::INSTANCE->world;
  }
  api.pushboolean(state, spawnEntityByName(world, entityId.c_str(), x, y, z) ? 1 : 0);
  return 1;
}
int luaWorldCountEntities(lua_State* state) {
  LuaApi& api = luaApi();
  if(api.type(state, 1) != kLuaTString) {
    api.pushinteger(state, 0);
    return 1;
  }
  World* world = LuaChunkContext::activeWorld();
  if(world == nullptr && client::Minecraft::INSTANCE != nullptr) {
    world = client::Minecraft::INSTANCE->world;
  }
  api.pushinteger(state, countEntitiesByName(world, luaString(state, 1, "").c_str()));
  return 1;
}
int luaWorldGetBlock(lua_State* state) {
  LuaApi& api = luaApi();
  if(api.gettop(state) < 3) {
    api.pushinteger(state, 0);
    return 1;
  }
  int isNumber = 0;
  const int x = static_cast<int>(api.tonumberx(state, 1, &isNumber));
  const int y = static_cast<int>(api.tonumberx(state, 2, &isNumber));
  const int z = static_cast<int>(api.tonumberx(state, 3, &isNumber));
  World* world = LuaChunkContext::activeWorld();
  if(world == nullptr && client::Minecraft::INSTANCE != nullptr) {
    world = client::Minecraft::INSTANCE->world;
  }
  api.pushinteger(state, getBlockIdAt(world, x, y, z));
  return 1;
}
#ifdef MINECRAFT_NATIVE_EXPORTS
int luaWorldSetCursor(lua_State* state) {
  LuaApi& api = luaApi();
  if(api.gettop(state) < 2) {
    api.pushboolean(state, 0);
    return 1;
  }
  int isNumber = 0;
  const int itemId = static_cast<int>(api.tonumberx(state, 1, &isNumber));
  const int count = static_cast<int>(api.tonumberx(state, 2, &isNumber));
  api.pushboolean(state, setPlayerCursorItem(itemId, count) ? 1 : 0);
  return 1;
}
int luaGuiFillRect(lua_State* state) {
  LuaApi& api = luaApi();
  if(!luaGuiDrawActive() || api.gettop(state) < 5) {
    return 0;
  }
  int isNumber = 0;
  const int x = static_cast<int>(api.tonumberx(state, 1, &isNumber));
  const int y = static_cast<int>(api.tonumberx(state, 2, &isNumber));
  const int w = static_cast<int>(api.tonumberx(state, 3, &isNumber));
  const int h = static_cast<int>(api.tonumberx(state, 4, &isNumber));
  const std::uint32_t color = luaArgb(state, 5);
  drawGuiFillRect(x, y, w, h, color);
  return 0;
}
int luaGuiDrawText(lua_State* state) {
  LuaApi& api = luaApi();
  if(!luaGuiDrawActive() || api.gettop(state) < 4 || api.type(state, 3) != kLuaTString) {
    return 0;
  }
  client::Minecraft* client = client::Minecraft::INSTANCE;
  if(client == nullptr || client->textRenderer == nullptr) {
    return 0;
  }
  int isNumber = 0;
  const int x = static_cast<int>(api.tonumberx(state, 1, &isNumber));
  const int y = static_cast<int>(api.tonumberx(state, 2, &isNumber));
  const std::string text = luaString(state, 3, "");
  const std::uint32_t color = luaArgb(state, 4);
  gl::GL11::glDisable(gl::GL11::GL_DEPTH_TEST);
  client->textRenderer->draw(text, x, y, std::bit_cast<int>(color));
  return 0;
}
int luaGuiDrawItem(lua_State* state) {
  LuaApi& api = luaApi();
  if(!luaGuiDrawActive() || api.gettop(state) < 4) {
    return 0;
  }
  client::Minecraft* client = client::Minecraft::INSTANCE;
  if(client == nullptr || client->textRenderer == nullptr) {
    return 0;
  }
  int isNumber = 0;
  const int x = static_cast<int>(api.tonumberx(state, 1, &isNumber));
  const int y = static_cast<int>(api.tonumberx(state, 2, &isNumber));
  const int itemId = static_cast<int>(api.tonumberx(state, 3, &isNumber));
  const int count = static_cast<int>(api.tonumberx(state, 4, &isNumber));
  if(itemId <= 0 || count <= 0) {
    return 0;
  }
  net::minecraft::ItemStack stack(itemId, count);
  gl::GL11::glEnable(gl::GL11::GL_LIGHTING);
  gl::GL11::glDisable(gl::GL11::GL_DEPTH_TEST);
  g_luaItemRenderer.renderGuiItem(*client->textRenderer, client->textureManager, stack, x, y);
  g_luaItemRenderer.renderGuiItemDecoration(*client->textRenderer, client->textureManager, stack, x, y);
  return 0;
}
int luaGuiTextWidth(lua_State* state) {
  LuaApi& api = luaApi();
  const std::string text = luaString(state, 1, "");
  int width = 0;
  if(client::Minecraft::INSTANCE != nullptr && client::Minecraft::INSTANCE->textRenderer != nullptr) {
    width = client::Minecraft::INSTANCE->textRenderer->getWidth(text);
  }
  api.pushinteger(state, static_cast<long long>(width));
  return 1;
}
int luaGuiDrawTexture(lua_State* state) {
  LuaApi& api = luaApi();
  if(!luaGuiDrawActive() || api.gettop(state) < 5) {
    return 0;
  }
  int isNumber = 0;
  const int textureId = static_cast<int>(api.tonumberx(state, 1, &isNumber));
  const int x = static_cast<int>(api.tonumberx(state, 2, &isNumber));
  const int y = static_cast<int>(api.tonumberx(state, 3, &isNumber));
  const int w = static_cast<int>(api.tonumberx(state, 4, &isNumber));
  const int h = static_cast<int>(api.tonumberx(state, 5, &isNumber));
  if(textureId <= 0) {
    return 0;
  }
  gl::GL11::glDisable(gl::GL11::GL_FOG);
  gl::GL11::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
  gl::GL11::glEnable(gl::GL11::GL_TEXTURE_2D);
  gl::GL11::glBindTexture(gl::GL11::GL_TEXTURE_2D, textureId);
  gl::GL11::glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
  client::render::Tessellator& tess = client::render::Tessellator::INSTANCE;
  client::gui::draw::texturedQuad(tess, x, y, x + w, y + h, 0.0f, 1.0f, 1.0f, 0.0f);
  return 0;
}
#endif
int luaParticlesSpawn(lua_State* state) {
  LuaApi& api = luaApi();
  if(api.type(state, 1) != kLuaTTable) {
    api.pushboolean(state, 0);
    return 1;
  }
  const int tableIndex = 1;
  const double x = luaFloatField(state, tableIndex, "x", 0.0f);
  const double y = luaFloatField(state, tableIndex, "y", 64.0f);
  const double z = luaFloatField(state, tableIndex, "z", 0.0f);
  const double vx = luaFloatField(state, tableIndex, "vx", 0.0f);
  const double vy = luaFloatField(state, tableIndex, "vy", 0.0f);
  const double vz = luaFloatField(state, tableIndex, "vz", 0.0f);
  const float scale = luaFloatField(state, tableIndex, "scale", 4.0f);
  const float red = luaFloatField(state, tableIndex, "r", 1.0f);
  const float green = luaFloatField(state, tableIndex, "g", 1.0f);
  const float blue = luaFloatField(state, tableIndex, "b", 1.0f);
  const int maxAge = luaIntField(state, tableIndex, "max_age", 40);
  const float gravity = luaFloatField(state, tableIndex, "gravity", 0.04f);
  api.pushboolean(state, spawnClientParticle(x, y, z, vx, vy, vz, scale, red, green, blue, maxAge, gravity) ? 1 : 0);
  return 1;
}
int luaScreenUiAddCenteredButton(lua_State* state) {
  ActiveScreenUi* session = activeScreenUi();
  if(session->context == nullptr || session->mod == nullptr || session->context->screen == nullptr) {
    return 0;
  }
  LuaApi& api = luaApi();
  if(api.gettop(state) < 2 || api.type(state, 1) != kLuaTNumber || api.type(state, 2) != kLuaTString) {
    return 0;
  }
  int isNumber = 0;
  const int y = static_cast<int>(api.tointegerx(state, 1, &isNumber));
  const std::string text = luaString(state, 2, "");
  const int ref = api.gettop(state) >= 3 ? retainButtonCallback(state, session->mod, 3) : kLuaNoRef;
  (void)session->context->addCenteredButton(y, text, [mod = session->mod, ref]() { invokeButtonCallback(mod, ref); });
  return 0;
}
int luaScreenUiAddButton(lua_State* state) {
  ActiveScreenUi* session = activeScreenUi();
  if(session->context == nullptr || session->mod == nullptr || session->context->screen == nullptr) {
    return 0;
  }
  LuaApi& api = luaApi();
  if(api.gettop(state) < 5 || api.type(state, 5) != kLuaTString) {
    return 0;
  }
  int isNumber = 0;
  const int x = static_cast<int>(api.tointegerx(state, 1, &isNumber));
  const int y = static_cast<int>(api.tointegerx(state, 2, &isNumber));
  const int width = static_cast<int>(api.tointegerx(state, 3, &isNumber));
  const int height = static_cast<int>(api.tointegerx(state, 4, &isNumber));
  const std::string text = luaString(state, 5, "");
  const int ref = api.gettop(state) >= 6 ? retainButtonCallback(state, session->mod, 6) : kLuaNoRef;
  (void)session->context->addButton(x, y, width, height, text, [mod = session->mod, ref]() {
    invokeButtonCallback(mod, ref);
  });
  return 0;
}
int luaScreenUiAddStackedCenteredButton(lua_State* state) {
  ActiveScreenUi* session = activeScreenUi();
  if(session->context == nullptr || session->mod == nullptr || session->context->screen == nullptr ||
     session->context->stackedButtonY == nullptr) {
    return 0;
  }
  LuaApi& api = luaApi();
  if(api.gettop(state) < 1 || api.type(state, 1) != kLuaTString) {
    return 0;
  }
  const std::string text = luaString(state, 1, "");
  const int ref = api.gettop(state) >= 2 ? retainButtonCallback(state, session->mod, 2) : kLuaNoRef;
  (void)session->context->addStackedCenteredButton(text, [mod = session->mod, ref]() {
    invokeButtonCallback(mod, ref);
  });
  session->stackedY = *session->context->stackedButtonY;
  return 0;
}
void pushScreenUiTable(lua_State* state) {
  LuaApi& api = luaApi();
  api.createtable(state, 0, 3);
  api.pushcclosure(state, luaScreenUiAddCenteredButton, 0);
  api.setfield(state, -2, "add_centered_button");
  api.pushcclosure(state, luaScreenUiAddButton, 0);
  api.setfield(state, -2, "add_button");
  api.pushcclosure(state, luaScreenUiAddStackedCenteredButton, 0);
  api.setfield(state, -2, "add_stacked_centered_button");
}
int luaScreenOpen(lua_State* state) {
  LuaApi& api = luaApi();
  if(client::Minecraft::INSTANCE == nullptr || api.type(state, 1) != kLuaTString) {
    return 0;
  }
  client::Minecraft::INSTANCE->setScreen(std::make_unique<LuaScreen>(luaString(state, 1, "")));
  return 0;
}
int luaScreenClose(lua_State* state) {
  (void)state;
  if(client::Minecraft::INSTANCE != nullptr) {
    client::Minecraft::INSTANCE->setScreen(nullptr);
  }
  return 0;
}
int luaScreenHostField(lua_State* state) {
  LuaApi& api = luaApi();
  std::string text;
  if(client::Minecraft::INSTANCE != nullptr && client::Minecraft::INSTANCE->currentScreen() != nullptr &&
     api.type(state, 1) == kLuaTString) {
    text = client::Minecraft::INSTANCE->currentScreen()->hostFieldText(luaString(state, 1, ""));
  }
  api.pushstring(state, text.c_str());
  return 1;
}
int luaScreenHostSetField(lua_State* state) {
  if(client::Minecraft::INSTANCE != nullptr && client::Minecraft::INSTANCE->currentScreen() != nullptr) {
    client::Minecraft::INSTANCE->currentScreen()->setHostFieldText(luaString(state, 1, ""), luaString(state, 2, ""));
  }
  return 0;
}
int luaScreenOpenHost(lua_State* state) {
  LuaApi& api = luaApi();
  if(api.type(state, 1) != kLuaTString) {
    return 0;
  }
  const std::string screenId = luaString(state, 1, "");
  std::unordered_map<std::string, std::string> fields;
  if(api.type(state, 2) == kLuaTTable) {
    api.pushnil(state);
    while(api.next(state, 2) != 0) {
      if(api.type(state, -2) == kLuaTString) {
        const char* key = api.tolstring(state, -2, nullptr);
        if(key != nullptr) {
          fields.emplace(key, luaString(state, -1, ""));
        }
      }
      api.settop(state, -2);
    }
  }
  (void)mod::openHostScreen(screenId, fields);
  return 0;
}
void pushHostFieldsTable(lua_State* state, client::gui::screen::Screen* screen) {
  LuaApi& api = luaApi();
  api.createtable(state, 0, 4);
  if(screen != nullptr) {
    screen->forEachHostField([state](std::string_view name, std::string_view value) {
      setField(state, std::string(name).c_str(), std::string(value));
    });
  }
}
int luaScreenAddField(lua_State* state) {
  ActiveLuaScreen* session = activeLuaScreen();
  LuaApi& api = luaApi();
  if(session->screen == nullptr || !session->initPhase || api.type(state, 1) != kLuaTString) {
    return 0;
  }
  int isNumber = 0;
  const std::string name = luaString(state, 1, "");
  const int x = static_cast<int>(api.tonumberx(state, 2, &isNumber));
  const int y = static_cast<int>(api.tonumberx(state, 3, &isNumber));
  const int width = static_cast<int>(api.tonumberx(state, 4, &isNumber));
  const int height = api.gettop(state) >= 5 && api.type(state, 5) == kLuaTNumber
                         ? static_cast<int>(api.tonumberx(state, 5, &isNumber))
                         : 20;
  std::string text;
  int maxLen = 0;
  bool numeric = false;
  bool allowSign = false;
  bool allowDot = false;
  if(api.type(state, 6) == kLuaTTable) {
    text = luaStringField(state, 6, "text", "");
    maxLen = luaIntField(state, 6, "max_len", 0);
    numeric = luaBoolField(state, 6, "numeric", false);
    allowSign = luaBoolField(state, 6, "signed", false);
    allowDot = luaBoolField(state, 6, "decimal", false);
  }
  session->screen->addField(name, x, y, width, height, text, maxLen, numeric, allowSign, allowDot);
  return 0;
}
int luaScreenFieldText(lua_State* state) {
  ActiveLuaScreen* session = activeLuaScreen();
  LuaApi& api = luaApi();
  std::string text;
  if(session->screen != nullptr && api.type(state, 1) == kLuaTString) {
    if(client::gui::widget::TextFieldWidget* field = session->screen->field(luaString(state, 1, ""))) {
      text = field->getText();
    }
  }
  api.pushstring(state, text.c_str());
  return 1;
}
int luaScreenSetFieldText(lua_State* state) {
  ActiveLuaScreen* session = activeLuaScreen();
  LuaApi& api = luaApi();
  if(session->screen != nullptr && api.type(state, 1) == kLuaTString) {
    if(client::gui::widget::TextFieldWidget* field = session->screen->field(luaString(state, 1, ""))) {
      field->setText(luaString(state, 2, ""));
    }
  }
  return 0;
}
int luaScreenSetFieldsVisible(lua_State* state) {
  ActiveLuaScreen* session = activeLuaScreen();
  LuaApi& api = luaApi();
  if(session->screen != nullptr) {
    session->screen->setFieldsVisible(api.toboolean(state, 1) != 0);
  }
  return 0;
}
int luaScreenAddButton(lua_State* state) {
  ActiveLuaScreen* session = activeLuaScreen();
  LuaApi& api = luaApi();
  if(session->screen == nullptr || !session->initPhase || api.gettop(state) < 5 ||
     api.type(state, 5) != kLuaTString) {
    return 0;
  }
  int isNumber = 0;
  const int x = static_cast<int>(api.tonumberx(state, 1, &isNumber));
  const int y = static_cast<int>(api.tonumberx(state, 2, &isNumber));
  const int width = static_cast<int>(api.tonumberx(state, 3, &isNumber));
  const int height = static_cast<int>(api.tonumberx(state, 4, &isNumber));
  const std::string text = luaString(state, 5, "");
  const int ref = api.gettop(state) >= 6 ? retainButtonCallback(state, session->mod, 6) : kLuaNoRef;
  ModHost::LoadedLuaMod* mod = session->mod;
  session->screen->addLuaButton(x, y, width, height, text, [mod, ref]() { invokeButtonCallback(mod, ref); });
  return 0;
}
int luaRenderDrawQuads(lua_State* state) {
  LuaApi& api = luaApi();
  if(!ModWorldDrawContext::active() || api.type(state, 1) != kLuaTTable ||
     client::Minecraft::INSTANCE == nullptr) {
    api.pushinteger(state, 0);
    return 1;
  }
  const int specIndex = 1;
  const std::string texturePath = luaStringField(state, specIndex, "texture", "");
  const bool textured = !texturePath.empty();
  const bool blend = luaBoolField(state, specIndex, "blend", true);
  const bool cull = luaBoolField(state, specIndex, "cull", false);
  const bool depthTest = luaBoolField(state, specIndex, "depth_test", true);
  const bool depthWrite = luaBoolField(state, specIndex, "depth_write", true);
  const float defaultR = std::clamp(luaFloatField(state, specIndex, "r", 1.0f), 0.0f, 1.0f);
  const float defaultG = std::clamp(luaFloatField(state, specIndex, "g", 1.0f), 0.0f, 1.0f);
  const float defaultB = std::clamp(luaFloatField(state, specIndex, "b", 1.0f), 0.0f, 1.0f);
  const float defaultA = std::clamp(luaFloatField(state, specIndex, "a", 1.0f), 0.0f, 1.0f);
  api.getfield(state, specIndex, "vertices");
  if(api.type(state, -1) != kLuaTTable) {
    pop(state, 1);
    api.pushinteger(state, 0);
    return 1;
  }
  const int verticesIndex = api.gettop(state);
  constexpr std::size_t kMaxVerticesPerBatch = 65536;
  const std::size_t rawCount = std::min(api.rawlen(state, verticesIndex), kMaxVerticesPerBatch);
  const std::size_t vertexCount = rawCount - rawCount % 4;
  if(vertexCount == 0) {
    pop(state, 1);
    api.pushinteger(state, 0);
    return 1;
  }
  for(std::size_t index = 1; index <= vertexCount; ++index) {
    api.rawgeti(state, verticesIndex, static_cast<long long>(index));
    const bool valid = api.type(state, -1) == kLuaTTable;
    pop(state, 1);
    if(!valid) {
      pop(state, 1);
      api.pushinteger(state, 0);
      return 1;
    }
  }
  const client::gl::AttribGuard stateScope(client::gl::GL11::GL_ALL_ATTRIB_BITS);
  client::render::platform::Lighting::turnOff();
  if(textured) {
    client::gl::GL11::glEnable(client::gl::GL11::GL_TEXTURE_2D);
    client::gl::GL11::glBindTexture(
        client::gl::GL11::GL_TEXTURE_2D,
        client::Minecraft::INSTANCE->textureManager.getTextureId(texturePath));
  } else {
    client::gl::GL11::glDisable(client::gl::GL11::GL_TEXTURE_2D);
  }
  if(blend) {
    client::gl::GL11::glEnable(client::gl::GL11::GL_BLEND);
    client::gl::GL11::glBlendFunc(client::gl::GL11::GL_SRC_ALPHA,
                                  client::gl::GL11::GL_ONE_MINUS_SRC_ALPHA);
  } else {
    client::gl::GL11::glDisable(client::gl::GL11::GL_BLEND);
  }
  if(cull) {
    client::gl::GL11::glEnable(client::gl::GL11::GL_CULL_FACE);
  } else {
    client::gl::GL11::glDisable(client::gl::GL11::GL_CULL_FACE);
  }
  if(depthTest) {
    client::gl::GL11::glEnable(client::gl::GL11::GL_DEPTH_TEST);
  } else {
    client::gl::GL11::glDisable(client::gl::GL11::GL_DEPTH_TEST);
  }
  client::gl::GL11::glDepthMask(depthWrite);
  client::gl::GL11::glShadeModel(client::gl::GL11::GL_FLAT);
  client::render::Tessellator& tessellator = client::render::Tessellator::INSTANCE;
  tessellator.startQuads();
  std::size_t emitted = 0;
  for(std::size_t index = 1; index <= vertexCount; ++index) {
    api.rawgeti(state, verticesIndex, static_cast<long long>(index));
    const int vertexIndex = api.gettop(state);
    const float r = std::clamp(luaFloatField(state, vertexIndex, "r", defaultR), 0.0f, 1.0f);
    const float g = std::clamp(luaFloatField(state, vertexIndex, "g", defaultG), 0.0f, 1.0f);
    const float b = std::clamp(luaFloatField(state, vertexIndex, "b", defaultB), 0.0f, 1.0f);
    const float a = std::clamp(luaFloatField(state, vertexIndex, "a", defaultA), 0.0f, 1.0f);
    tessellator.color(r, g, b, a);
    const double x = luaFloatField(state, vertexIndex, "x", 0.0f);
    const double y = luaFloatField(state, vertexIndex, "y", 0.0f);
    const double z = luaFloatField(state, vertexIndex, "z", 0.0f);
    if(textured) {
      const double u = luaFloatField(state, vertexIndex, "u", 0.0f);
      const double v = luaFloatField(state, vertexIndex, "v", 0.0f);
      tessellator.vertex(x, y, z, u, v);
    } else {
      tessellator.vertex(x, y, z);
    }
    ++emitted;
    pop(state, 1);
  }
  pop(state, 1);
  if(emitted >= 4) {
    tessellator.draw();
  }
  api.pushinteger(state, static_cast<long long>(emitted / 4));
  return 1;
}
void drawSphericalBillboard(client::render::Tessellator& tessellator, float yawDeg, float pitchDeg, double size,
                            float alpha, float brightness, float rotationXRad, float rotationYRad) {
  constexpr float kDegToRad = 0.017453292f;
  const float azimuthRad = yawDeg * kDegToRad;
  const float elevationRad = pitchDeg * kDegToRad;
  const double baseX = std::cos(static_cast<double>(elevationRad)) * std::sin(static_cast<double>(azimuthRad));
  const double baseY = std::sin(static_cast<double>(elevationRad));
  const double baseZ = -std::cos(static_cast<double>(elevationRad)) * std::cos(static_cast<double>(azimuthRad));
  const double rotationYCos = std::cos(static_cast<double>(rotationYRad));
  const double rotationYSin = std::sin(static_cast<double>(rotationYRad));
  const double vecX = baseX * rotationYCos + baseZ * rotationYSin;
  const double yawedZ = -baseX * rotationYSin + baseZ * rotationYCos;
  const double rotationXCos = std::cos(static_cast<double>(rotationXRad));
  const double rotationXSin = std::sin(static_cast<double>(rotationXRad));
  const double vecY = baseY * rotationXCos - yawedZ * rotationXSin;
  const double vecZ = baseY * rotationXSin + yawedZ * rotationXCos;
  const double starX = vecX * 100.0;
  const double starY = vecY * 100.0;
  const double starZ = vecZ * 100.0;
  double viewVec[3] = {-vecX, -vecY, -vecZ};
  double worldUp[3] = {0.0, 1.0, 0.0};
  double rightVec[3] = {viewVec[1] * worldUp[2] - viewVec[2] * worldUp[1],
                        viewVec[2] * worldUp[0] - viewVec[0] * worldUp[2],
                        viewVec[0] * worldUp[1] - viewVec[1] * worldUp[0]};
  double rightMag = std::sqrt(rightVec[0] * rightVec[0] + rightVec[1] * rightVec[1] + rightVec[2] * rightVec[2]);
  if(rightMag < 0.001) {
    worldUp[0] = 1.0;
    worldUp[1] = 0.0;
    worldUp[2] = 0.0;
    rightVec[0] = viewVec[1] * worldUp[2] - viewVec[2] * worldUp[1];
    rightVec[1] = viewVec[2] * worldUp[0] - viewVec[0] * worldUp[2];
    rightVec[2] = viewVec[0] * worldUp[1] - viewVec[1] * worldUp[0];
    rightMag = std::sqrt(rightVec[0] * rightVec[0] + rightVec[1] * rightVec[1] + rightVec[2] * rightVec[2]);
  }
  if(rightMag < 0.001) {
    return;
  }
  rightVec[0] /= rightMag;
  rightVec[1] /= rightMag;
  rightVec[2] /= rightMag;
  const double upVec[3] = {rightVec[1] * viewVec[2] - rightVec[2] * viewVec[1],
                           rightVec[2] * viewVec[0] - rightVec[0] * viewVec[2],
                           rightVec[0] * viewVec[1] - rightVec[1] * viewVec[0]};
  const double rdx = rightVec[0] * size;
  const double rdy = rightVec[1] * size;
  const double rdz = rightVec[2] * size;
  const double udx = upVec[0] * size;
  const double udy = upVec[1] * size;
  const double udz = upVec[2] * size;
  tessellator.color(brightness, brightness, brightness, alpha);
  tessellator.vertex(starX - rdx + udx, starY - rdy + udy, starZ - rdz + udz);
  tessellator.vertex(starX + rdx + udx, starY + rdy + udy, starZ + rdz + udz);
  tessellator.vertex(starX + rdx - udx, starY + rdy - udy, starZ + rdz - udz);
  tessellator.vertex(starX - rdx - udx, starY - rdy - udy, starZ - rdz - udz);
}
int luaRenderDrawBillboards(lua_State* state) {
  LuaApi& api = luaApi();
  if(!ModWorldDrawContext::active() || api.type(state, 1) != kLuaTTable) {
    api.pushinteger(state, 0);
    return 1;
  }
  const int tableIndex = 1;
  const float brightness = luaFloatField(state, tableIndex, "brightness", 1.0f);
  const float rotationXRad = luaFloatField(state, tableIndex, "rotation_x_rad", 0.0f);
  const float rotationYRad = luaFloatField(state, tableIndex, "rotation_y_rad", 0.0f);
  const std::string blendMode = luaStringField(state, tableIndex, "blend", "alpha");
  const bool depthTest = luaBoolField(state, tableIndex, "depth_test", false);
  const bool depthWrite = luaBoolField(state, tableIndex, "depth_write", false);
  if(brightness <= 0.0f) {
    api.pushinteger(state, 0);
    return 1;
  }
  api.getfield(state, tableIndex, "billboards");
  if(api.type(state, -1) != kLuaTTable) {
    api.getfield(state, tableIndex, "points");
  }
  if(api.type(state, -1) != kLuaTTable) {
    pop(state, 1);
    api.pushinteger(state, 0);
    return 1;
  }
  const int pointsIndex = api.gettop(state);
  const client::gl::AttribGuard stateScope(client::gl::GL11::GL_ALL_ATTRIB_BITS);
  client::render::platform::Lighting::turnOff();
  client::gl::GL11::glDisable(client::gl::GL11::GL_TEXTURE_2D);
  client::gl::GL11::glDisable(client::gl::GL11::GL_CULL_FACE);
  client::gl::GL11::glEnable(client::gl::GL11::GL_BLEND);
  client::gl::GL11::glBlendFunc(client::gl::GL11::GL_SRC_ALPHA,
                                blendMode == "additive" ? client::gl::GL11::GL_ONE
                                                        : client::gl::GL11::GL_ONE_MINUS_SRC_ALPHA);
  if(depthTest) {
    client::gl::GL11::glEnable(client::gl::GL11::GL_DEPTH_TEST);
  } else {
    client::gl::GL11::glDisable(client::gl::GL11::GL_DEPTH_TEST);
  }
  client::gl::GL11::glDepthMask(depthWrite);
  client::render::Tessellator& tessellator = client::render::Tessellator::INSTANCE;
  tessellator.startQuads();
  int emitted = 0;
  constexpr int kMaxBillboardsPerBatch = 65536;
  for(int index = 1; index <= kMaxBillboardsPerBatch; ++index) {
    api.rawgeti(state, pointsIndex, index);
    if(api.type(state, -1) != kLuaTTable) {
      pop(state, 1);
      break;
    }
    const int pointIndex = api.gettop(state);
    const float yawDeg = luaFloatField(state, pointIndex, "yaw_deg", luaFloatField(state, pointIndex, "az", 0.0f));
    const float pitchDeg = luaFloatField(state, pointIndex, "pitch_deg", luaFloatField(state, pointIndex, "el", 0.0f));
    const double size = static_cast<double>(luaFloatField(state, pointIndex, "size", 0.2f));
    const float alpha = luaFloatField(state, pointIndex, "alpha", 1.0f);
    drawSphericalBillboard(tessellator, yawDeg, pitchDeg, size, alpha, brightness, rotationXRad, rotationYRad);
    ++emitted;
    pop(state, 1);
  }
  pop(state, 1);
  if(emitted > 0) {
    tessellator.draw();
  }
  api.pushinteger(state, emitted);
  return 1;
}
int luaWorldSetTime(lua_State* state) {
  LuaApi& api = luaApi();
  int isNumber = 0;
  const long long tick = api.tointegerx(state, 1, &isNumber);
  if(isNumber == 0) {
    api.pushboolean(state, 0);
    return 1;
  }
  if(client::Minecraft::INSTANCE != nullptr && client::Minecraft::INSTANCE->world != nullptr) {
    client::Minecraft::INSTANCE->world->setTime(static_cast<std::uint64_t>(tick));
    api.pushboolean(state, 1);
    return 1;
  }
  api.pushboolean(state, 0);
  return 1;
}
int luaItemIds(lua_State* state) {
  LuaApi& api = luaApi();
  api.createtable(state, 0, 256);
  int index = 1;
  for(const Item* item : Item::ITEMS) {
    if(item == nullptr) {
      continue;
    }
    api.pushinteger(state, item->id);
    api.rawseti(state, -2, index++);
  }
  return 1;
}
#endif
void installMinecraftTable(lua_State* state, ModHost::LoadedLuaMod& mod) {
  LuaApi& api = luaApi();
  api.createtable(state, 0, 12);
  setField(state, "mod_id", mod.modId);
  bindModFunction(state, &mod, "log", luaLog);
  bindModFunction(state, &mod, "asset_path", luaAssetPath);
  bindModFunction(state, &mod, "read_asset", luaReadAsset);
  bindModFunction(state, &mod, "read_asset_bytes", luaReadAssetBytes);
  bindModFunction(state, &mod, "read_nbt_asset", luaReadNbtAsset);
  bindModFunction(state, &mod, "_read_storage", luaReadGameFile);
  bindModFunction(state, &mod, "_write_storage", luaWriteGameFile);
  bindModFunction(state, &mod, "_subscribe", luaOn);
  bindModFunction(state, &mod, "at_phase", luaAtPhase);
  bindModFunction(state, &mod, "_register_block", luaRegisterBlock);
  bindModFunction(state, &mod, "_register_shaped_recipe", luaRegisterShapedRecipe);
  bindFunctions(state, {
                           {"is_key_down", luaIsKeyDown},
                           {"key_code", luaKeyCode},
                       });
  pushFunctionTable(state, {{"utc_millis", luaTimeUtcMillis}});
  api.setfield(state, -2, "time");
  pushFunctionTable(state, {
                               {"set_block", luaChunkSetBlock},
                               {"fill", luaChunkFillBlock},
                               {"get_block", luaChunkGetBlock},
                               {"get_height", luaChunkGetHeight},
                           });
  api.setfield(state, -2, "chunk");
#ifdef MINECRAFT_NATIVE_EXPORTS
  pushFunctionTable(state, {
                               {"block_id", luaWorldBlockId},
                               {"get_block", luaWorldGetBlock},
                               {"random", luaWorldRandom},
                               {"is_night", luaWorldIsNight},
                               {"get_top_y", luaWorldTopSolidY},
                               {"player", luaWorldPlayer},
                               {"spawn_entity", luaWorldSpawnEntity},
                               {"count_entities", luaWorldCountEntities},
                               {"set_cursor", luaWorldSetCursor},
                               {"set_time", luaWorldSetTime},
                           });
  api.setfield(state, -2, "world");
  pushFunctionTable(state, {{"spawn", luaParticlesSpawn}});
  api.setfield(state, -2, "particles");
  pushFunctionTable(state, {
                               {"fill_rect", luaGuiFillRect},
                               {"draw_text", luaGuiDrawText},
                               {"draw_item", luaGuiDrawItem},
                               {"text_width", luaGuiTextWidth},
                               {"draw_texture", luaGuiDrawTexture},
                           });
  api.setfield(state, -2, "gui");
  pushFunctionTable(state, {
                               {"quads", luaRenderDrawQuads},
                               {"billboards", luaRenderDrawBillboards},
                           });
  api.setfield(state, -2, "render");
  pushFunctionTable(state, {{"get", luaOptionsGet}, {"keys", luaOptionsKeys}});
  api.setfield(state, -2, "options");
  pushFunctionTable(state, {{"ids", luaItemIds}});
  api.setfield(state, -2, "items");
  pushFunctionTable(state, {
                               {"open", luaScreenOpen},
                               {"close", luaScreenClose},
                               {"host_field", luaScreenHostField},
                               {"host_set_field", luaScreenHostSetField},
                               {"open_host", luaScreenOpenHost},
                               {"add_field", luaScreenAddField},
                               {"field_text", luaScreenFieldText},
                               {"set_field_text", luaScreenSetFieldText},
                               {"add_button", luaScreenAddButton},
                               {"set_fields_visible", luaScreenSetFieldsVisible},
                           });
  api.createtable(state, 0, 3);
  setField(state, "create_world", std::string(screen_ids::kCreateWorld));
  setField(state, "inventory", std::string(screen_ids::kInventory));
  setField(state, "detail_settings", std::string(screen_ids::kDetailSettings));
  api.setfield(state, -2, "ids");
  api.createtable(state, 0, 3);
  setField(state, "footer", std::string(screen_regions::kFooter));
  setField(state, "screen", std::string(screen_regions::kScreen));
  setField(state, "side_panel", std::string(screen_regions::kSidePanel));
  api.setfield(state, -2, "regions");
  api.setfield(state, -2, "screen");
  net::minecraft::mod::lua::installGenericModApi(state, mod);
#endif
  api.setglobal(state, "minecraft");
}
bool installLuaPrelude(lua_State* state, std::string& error) {
  LuaApi& api = luaApi();
  const std::string_view source = net::minecraft::mod::lua::kRuntimePrelude;
  int status = api.loadbufferx(state, source.data(), source.size(), "@minecraft/runtime.lua", "t");
  if(status == kLuaOk) {
    status = api.pcallk(state, 0, 0, 0, 0, nullptr);
  }
  if(status == kLuaOk) {
    return true;
  }
  const char* message = api.tolstring(state, -1, nullptr);
  error = message != nullptr ? message : "failed to initialize the Lua runtime prelude";
  api.settop(state, 0);
  return false;
}
template <typename Fill, typename Apply>
void callLuaEvent(const std::shared_ptr<ModHost::LoadedLuaMod>& mod, int ref, Fill fill, Apply apply) {
  LuaApi& api = luaApi();
  if(!api.ready() || mod == nullptr) {
    return;
  }
  const std::lock_guard<std::recursive_mutex> lock(mod->stateMutex);
  if(!mod->active || mod->state == nullptr) {
    return;
  }
  auto* state = static_cast<lua_State*>(mod->state);
  api.rawgeti(state, kLuaRegistryIndex, ref);
  if(api.type(state, -1) != kLuaTFunction) {
    api.settop(state, -2);
    return;
  }
  api.createtable(state, 0, 12);
  fill(state);
  const int eventIndex = api.gettop(state);
  api.pushvalue(state, -2);
  api.pushvalue(state, -2);
  const int status = api.pcallk(state, 1, 1, 0, 0, nullptr);
  if(status != kLuaOk) {
    const char* error = api.tolstring(state, -1, nullptr);
    runtimeLog(mod->modId, "error", error != nullptr ? error : "Lua callback failed");
    api.settop(state, -4);
    return;
  }
  if(api.type(state, -1) != kLuaTTable) {
    api.settop(state, -2);
    api.pushvalue(state, eventIndex);
  }
  apply(state);
  api.settop(state, eventIndex - 2);
}
void subscribeWorldColorEvent(const std::shared_ptr<ModHost::LoadedLuaMod>& mod, int ref, int priority) {
  hooks().subscribe<WorldColorEvent>(priority, [mod, ref](WorldColorEvent& e) {
    callLuaEvent(
        mod, ref,
        [&e](lua_State* state) {
          setField(state, "partial_ticks", e.partialTicks);
          setField(state, "r", e.color.x);
          setField(state, "g", e.color.y);
          setField(state, "b", e.color.z);
          setField(state, "kind", e.kind == WorldColorKind::Sky ? "sky" : "fog");
          setWorldContextFields(state, e.world);
#ifdef MINECRAFT_NATIVE_EXPORTS
          if(e.world != nullptr) {
            setField(state, "celestial", static_cast<double>(normalizedCelestial(e.world, e.partialTicks)));
            setField(state, "world_time", static_cast<double>(e.world->getTime() % 24000ULL));
            setField(state, "is_night", worldIsNight(e.world));
          }
#endif
        },
        [&e](lua_State* state) {
          const auto component = [state](const char* name, double fallback) {
            const float value = luaFloatField(state, -1, name, static_cast<float>(fallback));
            return std::isfinite(value) ? static_cast<double>(std::clamp(value, 0.0f, 1.0f)) : fallback;
          };
          e.color.x = component("r", e.color.x);
          e.color.y = component("g", e.color.y);
          e.color.z = component("b", e.color.z);
        });
  });
}
void subscribeLuaCallback(const std::shared_ptr<ModHost::LoadedLuaMod>& mod,
                          const ModHost::LoadedLuaMod::Callback& callback) {
  const std::string event = toLowerCopy(callback.event);
  const int ref = callback.functionRef;
  const int priority = callback.priority;
  if(event == "client_tick") {
    hooks().subscribe<ClientTickEvent>(priority, [mod, ref](ClientTickEvent& e) {
      callLuaEvent(
          mod, ref,
          [&e](lua_State* state) { setClientTickFields(state, e); },
          [](lua_State*) {});
    });
  } else if(event == "key_press") {
    hooks().subscribe<KeyPressEvent>(priority, [mod, ref](KeyPressEvent& e) {
      callLuaEvent(
          mod, ref,
          [&e](lua_State* state) {
            setField(state, "key", e.key);
            setField(state, "pressed", e.pressed);
            setField(state, "repeat", e.repeat);
            setField(state, "handled", e.handled);
          },
          [&e](lua_State* state) { e.handled = luaBoolField(state, -1, "handled", e.handled); });
    });
  } else if(event == "mouse_button") {
    hooks().subscribe<MouseButtonEvent>(priority, [mod, ref](MouseButtonEvent& e) {
      callLuaEvent(
          mod, ref,
          [&e](lua_State* state) {
            setField(state, "button", e.button);
            setField(state, "pressed", e.pressed);
            setField(state, "handled", e.handled);
          },
          [&e](lua_State* state) { e.handled = luaBoolField(state, -1, "handled", e.handled); });
    });
  } else if(event == "fov") {
    hooks().subscribe<FovEvent>(priority, [mod, ref](FovEvent& e) {
      callLuaEvent(
          mod, ref,
          [&e](lua_State* state) {
            setField(state, "tick_delta", e.tickDelta);
            setField(state, "fov", e.fov);
          },
          [&e](lua_State* state) { e.fov = luaFloatField(state, -1, "fov", e.fov); });
    });
  } else if(event == "player_travel") {
    hooks().subscribe<PlayerTravelEvent>(priority, [mod, ref](PlayerTravelEvent& e) {
      callLuaEvent(
          mod, ref,
          [&e](lua_State* state) {
            setField(state, "sideways", e.sideways);
            setField(state, "forward", e.forward);
            setField(state, "speed_multiplier", e.speedMultiplier);
            setField(state, "has_player", e.player != nullptr);
#ifdef MINECRAFT_NATIVE_EXPORTS
            const client::Minecraft* client = client::Minecraft::INSTANCE;
            const bool isLocal = e.player != nullptr && client != nullptr &&
                                 (client->player == e.player || client->camera == e.player);
            setField(state, "is_local_player", isLocal);
#else
                                                                                            setField(state, "is_local_player", false);
#endif
          },
          [&e](lua_State* state) {
            e.sideways = luaFloatField(state, -1, "sideways", e.sideways);
            e.forward = luaFloatField(state, -1, "forward", e.forward);
            e.speedMultiplier = luaFloatField(state, -1, "speed_multiplier", e.speedMultiplier);
          });
    });
  } else if(event == "tick_rate") {
    hooks().subscribe<TickRateEvent>(priority, [mod, ref](TickRateEvent& e) {
      callLuaEvent(
          mod, ref,
          [&e](lua_State* state) {
            setField(state, "target_tps", e.targetTps);
            setField(state, "tps_scale", e.tpsScale);
          },
          [&e](lua_State* state) {
            e.targetTps = luaFloatField(state, -1, "target_tps", e.targetTps);
            e.tpsScale = luaFloatField(state, -1, "tps_scale", e.tpsScale);
          });
    });
  } else if(event == "world_start") {
    hooks().subscribe<WorldStartEvent>(priority, [mod, ref](WorldStartEvent& e) {
      callLuaEvent(
          mod, ref,
          [&e](lua_State* state) {
            setField(state, "save_name", e.saveName != nullptr ? *e.saveName : std::string());
            setField(state, "new_world", e.newWorld);
          },
          [](lua_State*) {});
    });
  } else if(event == "world_open") {
    hooks().subscribe<WorldOpenEvent>(priority, [mod, ref](WorldOpenEvent& e) {
      callLuaEvent(
          mod, ref,
          [&e](lua_State* state) {
            setField(state, "save_name", e.saveName != nullptr ? *e.saveName : std::string());
            setField(state, "new_world", e.newWorld);
            if(e.options != nullptr) {
              pushStringMap(state, *e.options);
            } else {
              pushStringMap(state, {});
            }
            luaApi().setfield(state, -2, "options");
          },
          [](lua_State*) {});
    });
  } else if(event == "world_tick") {
    hooks().subscribe<WorldTickEvent>(priority, [mod, ref](WorldTickEvent& e) {
      callLuaEvent(
          mod, ref,
          [&e](lua_State* state) {
            setField(state, "client_world", e.clientWorld);
            setField(state, "before", e.before);
          },
          [](lua_State*) {});
    });
  } else if(event == "create_world") {
    hooks().subscribe<CreateWorldEvent>(priority, [mod, ref](CreateWorldEvent& e) {
      callLuaEvent(
          mod, ref,
          [&e](lua_State* state) {
            setField(state, "save_name", e.saveName != nullptr ? *e.saveName : std::string());
            setField(state, "seed", e.seed);
            setField(state, "canceled", e.canceled);
            pushStringMap(state, e.options);
            luaApi().setfield(state, -2, "options");
          },
          [&e](lua_State* state) {
            e.canceled = luaBoolField(state, -1, "canceled", e.canceled);
            readStringMapField(state, -1, "options", e.options);
          });
    });
  } else if(event == "block_interact") {
    hooks().subscribe<BlockInteractEvent>(priority, [mod, ref](BlockInteractEvent& e) {
      if(e.world != nullptr && e.world->isRemote()) {
        return;
      }
      callLuaEvent(
          mod, ref,
          [&e](lua_State* state) {
            setField(state, "x", e.x);
            setField(state, "y", e.y);
            setField(state, "z", e.z);
            setField(state, "block_id", getBlockIdAt(e.world, e.x, e.y, e.z));
            setField(state, "side", e.side);
            setField(state, "right_click", e.rightClick);
            setField(state, "canceled", e.canceled);
            setField(state, "handled", e.handled);
            setField(state, "has_player", e.player != nullptr);
            setField(state, "has_item", e.stack != nullptr && !e.stack->empty());
            if(e.stack != nullptr && !e.stack->empty()) {
              setField(state, "item_id", e.stack->itemId);
              setField(state, "item_count", e.stack->count);
              setField(state, "item_damage", e.stack->damage);
              setField(state, "item_max_damage", e.stack->getMaxDamage());
              setField(state, "item_damageable", e.stack->isDamageable());
            }
          },
          [&e](lua_State* state) {
            e.canceled = luaBoolField(state, -1, "canceled", e.canceled);
            e.handled = luaBoolField(state, -1, "handled", e.handled);
            if(e.stack != nullptr && e.stack->isDamageable()) {
              e.stack->damage = std::clamp(luaIntField(state, -1, "item_damage", e.stack->damage), 0,
                                           e.stack->getMaxDamage());
            }
          });
    });
  } else if(event == "entity_interact") {
    hooks().subscribe<EntityInteractEvent>(priority, [mod, ref](EntityInteractEvent& e) {
      if(e.player != nullptr && e.player->world != nullptr && e.player->world->isRemote()) {
        return;
      }
      callLuaEvent(
          mod, ref,
          [&e](lua_State* state) {
            setField(state, "attack", e.attack);
            setField(state, "canceled", e.canceled);
            setField(state, "handled", e.handled);
            setField(state, "has_player", e.player != nullptr);
            setField(state, "has_target", e.target != nullptr);
          },
          [&e](lua_State* state) {
            e.canceled = luaBoolField(state, -1, "canceled", e.canceled);
            e.handled = luaBoolField(state, -1, "handled", e.handled);
          });
    });
  } else if(event == "attack_damage") {
    hooks().subscribe<AttackDamageEvent>(priority, [mod, ref](AttackDamageEvent& e) {
      callLuaEvent(
          mod, ref,
          [&e](lua_State* state) {
            setField(state, "damage", e.damage);
            setField(state, "critical", e.critical);
            setField(state, "canceled", e.canceled);
            setField(state, "fall_distance", e.fallDistance);
            setField(state, "on_ground", e.onGround);
            setField(state, "target_x", e.targetX);
            setField(state, "target_y", e.targetY);
            setField(state, "target_z", e.targetZ);
            setField(state, "has_player", e.player != nullptr);
            setField(state, "has_target", e.target != nullptr);
          },
          [&e](lua_State* state) {
            e.damage = luaIntField(state, -1, "damage", e.damage);
            e.critical = luaBoolField(state, -1, "critical", e.critical);
            e.canceled = luaBoolField(state, -1, "canceled", e.canceled);
          });
    });
  } else if(event == "world_color") {
    subscribeWorldColorEvent(mod, ref, priority);
  } else if(event == "world_render") {
    hooks().subscribe<WorldRenderEvent>(priority, [mod, ref](WorldRenderEvent& e) {
#ifdef MINECRAFT_NATIVE_EXPORTS
      ScopedModWorldDrawContext worldDrawScope{e.world, e.tickDelta};
      const client::gl::MatrixGuard matrixScope;
      const client::gl::AttribGuard stateScope(client::gl::GL11::GL_ALL_ATTRIB_BITS);
#endif
      callLuaEvent(
          mod, ref,
          [&e](lua_State* state) {
            setField(state, "tick_delta", e.tickDelta);
            setField(state, "stage", renderStageName(e.stage));
            setField(state, "moment", renderMomentName(e.moment));
            setField(state, "cancel_vanilla", e.cancelVanilla);
            setField(state, "vanilla_stage_ran", e.vanillaStageRan);
            setField(state, "celestial_angle", static_cast<double>(e.celestialAngle));
            setField(state, "sky_yaw_deg", static_cast<double>(e.skyYawDegrees));
            setField(state, "star_brightness", static_cast<double>(e.starBrightness));
            setField(state, "rain_strength", static_cast<double>(e.rainStrength));
            setField(state, "stars_enabled", e.starsEnabled);
            setField(state, "astronomy_enabled", e.astronomyEnabled);
            setField(state, "astronomy_utc_millis", e.astronomyUtcMillis);
            setField(state, "observer_latitude_deg", static_cast<double>(e.observerLatitudeDegrees));
            setField(state, "observer_longitude_deg", static_cast<double>(e.observerLongitudeDegrees));
            setWorldContextFields(state, e.world);
            double cameraX = 0.0;
            double cameraY = 64.0;
            double cameraZ = 0.0;
#ifdef MINECRAFT_NATIVE_EXPORTS
            if(e.camera != nullptr) {
              cameraX = e.camera->lastTickX + (e.camera->x - e.camera->lastTickX) * static_cast<double>(e.tickDelta);
              cameraY = e.camera->lastTickY + (e.camera->y - e.camera->lastTickY) * static_cast<double>(e.tickDelta);
              cameraZ = e.camera->lastTickZ + (e.camera->z - e.camera->lastTickZ) * static_cast<double>(e.tickDelta);
            }
            if(e.world != nullptr) {
              setField(state, "world_time", static_cast<double>(e.world->getTime() % 24000ULL));
              setField(state, "celestial", static_cast<double>(normalizedCelestial(e.world, e.tickDelta)));
              setField(state, "is_night", worldIsNight(e.world));
            }
            if(e.stage == WorldRenderStage::Clouds && e.world != nullptr && e.world->dimension != nullptr) {
              float cloudBaseHeight = e.world->dimension->getCloudHeight() - static_cast<float>(cameraY) + 0.33f;
              if(client::Minecraft* client = client::Minecraft::INSTANCE; client != nullptr) {
                cloudBaseHeight = client::option::cloudHeightOffset(
                    cloudBaseHeight, client::option::resolve(client->options));
              }
              setField(state, "cloud_base_height", cloudBaseHeight);
            }
#endif
            setField(state, "camera_x", cameraX);
            setField(state, "camera_y", cameraY);
            setField(state, "camera_z", cameraZ);
          },
          [&e](lua_State* state) {
            e.cancelVanilla = luaBoolField(state, -1, "cancel_vanilla", e.cancelVanilla);
            if(e.stage == WorldRenderStage::Sky && e.moment == RenderHookMoment::Before) {
              e.celestialAngle = luaFloatField(state, -1, "celestial_angle", e.celestialAngle);
              e.skyYawDegrees = luaFloatField(state, -1, "sky_yaw_deg", e.skyYawDegrees);
              e.astronomyEnabled = luaBoolField(state, -1, "astronomy_enabled", e.astronomyEnabled);
              e.astronomyUtcMillis =
                  luaDoubleField(state, -1, "astronomy_utc_millis", e.astronomyUtcMillis);
              e.observerLatitudeDegrees =
                  luaFloatField(state, -1, "observer_latitude_deg", e.observerLatitudeDegrees);
              e.observerLongitudeDegrees =
                  luaFloatField(state, -1, "observer_longitude_deg", e.observerLongitudeDegrees);
            }
          });
    });
  } else if(event == "chunk_generation") {
    hooks().subscribe<world::gen::ChunkGenerationEvent>(priority, [mod, ref](world::gen::ChunkGenerationEvent& e) {
      if(e.context.world != nullptr && e.context.world->isRemote()) {
        return;
      }
#ifdef MINECRAFT_NATIVE_EXPORTS
      LuaChunkContext::Scope scope(e.context.chunk, e.context.world, e.context.chunkX, e.context.chunkZ,
                                   chunkWriteModeForStage(e.stage));
#endif
      callLuaEvent(
          mod, ref,
          [&e](lua_State* state) {
            setField(state, "stage", chunkStageName(e.stage));
            setField(state, "moment", e.moment == world::gen::HookMoment::Before ? "before" : "after");
            setField(state, "cancel_vanilla", e.cancelVanilla);
            setField(state, "vanilla_stage_ran", e.vanillaStageRan);
            setField(state, "world_seed", static_cast<std::int64_t>(e.context.worldSeed));
            setWorldContextFields(state, e.context.world);
            setField(state, "mod_generation", e.context.modGeneration);
            setField(state, "is_overworld", e.context.overworld);
            setChunkContextFields(state);
          },
          [&e](lua_State* state) {
            e.cancelVanilla = luaBoolField(state, -1, "cancel_vanilla", e.cancelVanilla);
          });
    });
  } else if(event == "screen_region") {
    hooks().subscribe<ScreenRegionEvent>(priority, [mod, ref](ScreenRegionEvent& e) {
#ifdef MINECRAFT_NATIVE_EXPORTS
      const bool renderPhase = e.phase == ScreenRegionPhase::Render;
      const ScopedLuaGuiDraw drawScope(renderPhase);
#endif
      callLuaEvent(
          mod, ref,
          [&e](lua_State* state) {
            const char* phaseName = "render";
            if(e.phase == ScreenRegionPhase::MouseClick) {
              phaseName = "mouse_click";
            } else if(e.phase == ScreenRegionPhase::MouseScroll) {
              phaseName = "mouse_scroll";
            }
            setField(state, "phase_name", phaseName);
            setField(state, "screen_id", std::string(e.screenId));
            setField(state, "region", std::string(e.region));
            setField(state, "mouse_x", e.mouseX);
            setField(state, "mouse_y", e.mouseY);
            setField(state, "button", e.button);
            setField(state, "scroll_delta", e.scrollDelta);
            setField(state, "x", e.x);
            setField(state, "y", e.y);
            setField(state, "width", e.width);
            setField(state, "height", e.height);
            setField(state, "handled", e.handled);
          },
          [&e](lua_State* state) {
            e.handled = luaBoolField(state, -1, "handled", e.handled);
            e.width = luaIntField(state, -1, "width", e.width);
            e.height = luaIntField(state, -1, "height", e.height);
          });
    });
  } else if(event == "world_spawn_search") {
    hooks().subscribe<WorldSpawnSearchEvent>(priority, [mod, ref](WorldSpawnSearchEvent& e) {
      if(e.world != nullptr && e.world->isRemote()) {
        return;
      }
      callLuaEvent(
          mod, ref,
          [&e](lua_State* state) {
            setField(state, "x", e.x);
            setField(state, "y", e.y);
            setField(state, "z", e.z);
            setField(state, "resolved", e.resolved);
            setWorldContextFields(state, e.world);
          },
          [&e](lua_State* state) {
            e.x = luaIntField(state, -1, "x", e.x);
            e.y = luaIntField(state, -1, "y", e.y);
            e.z = luaIntField(state, -1, "z", e.z);
            e.resolved = luaBoolField(state, -1, "resolved", e.resolved);
          });
    });
  } else if(event == "screen_ui") {
#ifdef MINECRAFT_NATIVE_EXPORTS
    hooks().subscribe<ScreenUiEvent>(priority, [mod, ref](ScreenUiEvent& e) {
      if(e.context == nullptr || e.context->screen == nullptr) {
        return;
      }
      ActiveScreenUi* session = activeScreenUi();
      session->context = e.context;
      session->mod = mod.get();
      session->stackedY = e.context->stackedButtonY != nullptr ? *e.context->stackedButtonY : 0;
      session->trackStacked = e.context->stackedButtonY != nullptr;
      callLuaEvent(
          mod, ref,
          [&e](lua_State* state) {
            setField(state, "screen_id", std::string(e.context->screenId));
            setField(state, "region", std::string(e.context->region));
#ifdef MINECRAFT_NATIVE_EXPORTS
            pushHostFieldsTable(state, e.context->screen);
            luaApi().setfield(state, -2, "host_fields");
#endif
            pushScreenUiTable(state);
            luaApi().setfield(state, -2, "ui");
          },
          [](lua_State*) {});
      if(session->trackStacked && e.context->stackedButtonY != nullptr) {
        *e.context->stackedButtonY = session->stackedY;
      }
      session->context = nullptr;
      session->mod = nullptr;
      session->trackStacked = false;
    });
#else
    runtimeLog(mod->modId, "warn", "unsupported Lua hook event: " + callback.event);
#endif
  } else if(event == "screen_event") {
#ifdef MINECRAFT_NATIVE_EXPORTS
    hooks().subscribe<LuaScreenEvent>(priority, [mod, ref](LuaScreenEvent& e) {
      if(e.screen == nullptr) {
        return;
      }
      static constexpr const char* kPhaseNames[] = {
          "init",
          "render",
          "tick",
          "key",
          "mouse",
          "scroll",
          "close",
      };
      activeLuaScreen()->mod = mod.get();
      callLuaEvent(
          mod, ref,
          [&e](lua_State* state) {
            setField(state, "screen_id", e.screen->id());
            setField(state, "phase", kPhaseNames[static_cast<int>(e.phase)]);
            setField(state, "width", e.screen->width());
            setField(state, "height", e.screen->height());
            setField(state, "mouse_x", e.mouseX);
            setField(state, "mouse_y", e.mouseY);
            setField(state, "x", e.mouseX);
            setField(state, "y", e.mouseY);
            setField(state, "tick_delta", e.tickDelta);
            setField(state, "key", e.keyCode);
            setField(state, "char", static_cast<int>(static_cast<unsigned char>(e.character)));
            setField(state, "button", e.button);
            setField(state, "released", e.released);
            setField(state, "delta", e.scrollDelta);
            setField(state, "handled", e.handled);
          },
          [&e](lua_State* state) { e.handled = luaBoolField(state, -1, "handled", e.handled); });
      activeLuaScreen()->mod = nullptr;
    });
#else
    runtimeLog(mod->modId, "warn", "unsupported Lua hook event: " + callback.event);
#endif
  } else {
    runtimeLog(mod->modId, "warn", "unsupported Lua hook event: " + callback.event);
  }
}
bool loadLuaMod(ModPackage& info, std::vector<std::shared_ptr<ModHost::LoadedLuaMod>>& loadedMods) {
  if(info.entry.empty()) {
    info.active = info.resourceOverlay;
    return true;
  }
  if(!info.runtimeScript) {
    info.error = "Only Lua script entries are supported";
    info.active = info.resourceOverlay;
    return false;
  }
  if(!isSafeRelativePath(info.entry)) {
    info.error = "Unsafe script path";
    info.active = info.resourceOverlay;
    return false;
  }
  LuaApi& api = luaApi();
  if(!api.ready()) {
    info.error = "Lua runtime unavailable";
    info.active = info.resourceOverlay;
    return false;
  }
  const std::filesystem::path scriptPath = info.rootPath / std::filesystem::path(info.entry);
  if(!std::filesystem::is_regular_file(scriptPath)) {
    info.error = "Missing Lua script: " + scriptPath.string();
    info.active = info.resourceOverlay;
    return false;
  }
  lua_State* state = api.newstate();
  if(state == nullptr) {
    info.error = "Failed to create Lua state";
    info.active = info.resourceOverlay;
    return false;
  }
  api.openlibs(state);
  auto mod = std::make_shared<ModHost::LoadedLuaMod>();
  mod->modId = info.id;
  mod->state = state;
  installMinecraftTable(state, *mod);
  if(!installLuaPrelude(state, info.error)) {
    api.close(state);
    info.active = info.resourceOverlay;
    return false;
  }
  const std::string script = scriptPath.string();
  int status = api.loadfilex(state, script.c_str(), "t");
  if(status == kLuaOk) {
    status = api.pcallk(state, 0, 0, 0, 0, nullptr);
  }
  if(status != kLuaOk) {
    const char* error = api.tolstring(state, -1, nullptr);
    info.error = error != nullptr ? error : "Lua script failed to load";
    api.close(state);
    info.active = info.resourceOverlay;
    return false;
  }
  mod->active = true;
  for(const auto& callback : mod->callbacks) {
    subscribeLuaCallback(mod, callback);
  }
  loadedMods.push_back(std::move(mod));
  info.active = true;
  runtimeLog(info.id, "info", "loaded " + info.name + " " + info.version);
  return true;
}
} // namespace
ModHost& host() {
  static ModHost value;
  return value;
}
const std::vector<std::shared_ptr<ModHost::LoadedLuaMod>>& loadedLuaMods() {
  return host().loadedMods();
}
std::filesystem::path ModHost::defaultRunDirectory() {
  const char* appData = std::getenv("APPDATA");
  if(appData != nullptr && *appData != '\0') {
    return std::filesystem::path(appData) / ".minecraft";
  }
  const char* home = std::getenv("USERPROFILE");
  if(home != nullptr && *home != '\0') {
    return std::filesystem::path(home) / ".minecraft";
  }
  return std::filesystem::current_path() / ".minecraft";
}
void ModHost::initialize(const std::filesystem::path& runDirectory) {
  if(initialized_) {
    return;
  }
  runDirectory_ = runDirectory;
  modsDirectory_ = runDirectory_ / "mods";
  cacheDirectory_ = modsDirectory_ / ".cache";
  std::filesystem::create_directories(modsDirectory_);
  std::filesystem::create_directories(cacheDirectory_);
  initialized_ = true;
  loadStateFile();
  rescan();
}
void ModHost::shutdown() {
  LuaApi* api = loadedLuaMods_.empty() ? nullptr : &luaApi();
  for(const std::shared_ptr<LoadedLuaMod>& mod : loadedLuaMods_) {
    if(mod == nullptr) {
      continue;
    }
    const std::lock_guard<std::recursive_mutex> lock(mod->stateMutex);
    mod->active = false;
    if(api != nullptr && api->ready() && mod->state != nullptr) {
      auto* state = static_cast<lua_State*>(mod->state);
      for(const LoadedLuaMod::Callback& callback : mod->callbacks) {
        if(callback.functionRef != kLuaNoRef) {
          api->unref(state, kLuaRegistryIndex, callback.functionRef);
        }
      }
      for(const int ref : mod->buttonCallbackRefs) {
        if(ref != kLuaNoRef) {
          api->unref(state, kLuaRegistryIndex, ref);
        }
      }
      mod->buttonCallbackRefs.clear();
#ifdef MINECRAFT_NATIVE_EXPORTS
      if(client::Minecraft::INSTANCE != nullptr) {
        for(const int textureId : mod->ownedTextureIds) {
          client::Minecraft::INSTANCE->textureManager.deleteTexture(textureId);
        }
      }
#endif
      mod->ownedTextureIds.clear();
      api->close(state);
      mod->state = nullptr;
    }
  }
  loadedLuaMods_.clear();
  packageMods_.clear();
  packageModsLoaded_ = false;
  initialized_ = false;
}
void ModHost::loadStateFile() {
  savedEnabledStates_.clear();
  const std::filesystem::path statePath = modsDirectory_ / "mod-state.cfg";
  std::ifstream input(statePath);
  if(!input.is_open()) {
    return;
  }
  std::string line;
  while(std::getline(input, line)) {
    line = trimCopy(line);
    if(line.empty() || line.starts_with('#') || line.starts_with(';')) {
      continue;
    }
    const std::size_t equals = line.find('=');
    if(equals == std::string::npos) {
      continue;
    }
    const std::string id = trimCopy(line.substr(0, equals));
    const std::string value = toLowerCopy(trimCopy(line.substr(equals + 1)));
    if(!id.empty()) {
      savedEnabledStates_[id] = value == "1" || value == "true" || value == "yes" || value == "on";
    }
  }
}
void ModHost::saveStateFile() const {
  std::vector<std::string> ids;
  ids.reserve(savedEnabledStates_.size());
  for(const auto& [id, _] : savedEnabledStates_) {
    ids.push_back(id);
  }
  std::sort(ids.begin(), ids.end());
  std::ostringstream output;
  output << "# Minecraft Native Lua mod enable state\n";
  for(const std::string& id : ids) {
    const auto it = savedEnabledStates_.find(id);
    if(it != savedEnabledStates_.end()) {
      output << id << '=' << (it->second ? "1" : "0") << '\n';
    }
  }
  (void)writeFileText(modsDirectory_ / "mod-state.cfg", output.str());
}
ModPackage* ModHost::findPackageMod(const std::string& modId) {
  for(ModPackage& mod : packageMods_) {
    if(mod.id == modId) {
      return &mod;
    }
  }
  return nullptr;
}
const ModPackage* ModHost::findPackageMod(const std::string& modId) const {
  for(const ModPackage& mod : packageMods_) {
    if(mod.id == modId) {
      return &mod;
    }
  }
  return nullptr;
}
bool ModHost::isEnabled(const std::string& modId, bool enabledByDefault) const {
  const auto it = savedEnabledStates_.find(modId);
  if(it != savedEnabledStates_.end()) {
    return it->second;
  }
  if(const ModPackage* pkg = findPackageMod(modId)) {
    return pkg->configuredEnabled;
  }
  return enabledByDefault;
}
bool ModHost::setEnabled(const std::string& modId, bool enabled) {
  savedEnabledStates_[modId] = enabled;
  if(ModPackage* mod = findPackageMod(modId)) {
    mod->configuredEnabled = enabled;
  }
  saveStateFile();
  return true;
}
void ModHost::rescan() {
  if(!initialized_) {
    return;
  }
  loadStateFile();
  std::unordered_map<std::string, ModPackage> previous;
  for(const ModPackage& mod : packageMods_) {
    previous.emplace(mod.id, mod);
  }
  packageMods_.clear();
  std::set<std::string> seenIds;
  if(std::filesystem::exists(modsDirectory_) && std::filesystem::is_directory(modsDirectory_)) {
    for(auto it = std::filesystem::recursive_directory_iterator(modsDirectory_);
        it != std::filesystem::recursive_directory_iterator(); ++it) {
      const auto& entry = *it;
      const std::filesystem::path path = entry.path();
      const std::string filename = path.filename().string();
      if(entry.is_directory()) {
        if(filename == ".cache") {
          it.disable_recursion_pending();
          continue;
        }
        const std::filesystem::path manifestPath = path / "mod.json";
        if(!std::filesystem::is_regular_file(manifestPath)) {
          continue;
        }
        ModPackage modInfo;
        const bool discovered =
            parseManifestJson(readFileText(manifestPath), modInfo, path, ModPackageSource::Directory, "mod.json: ");
        if(discovered) {
          modInfo.rootPath = path;
          modInfo.resourceOverlay = std::filesystem::is_directory(modInfo.rootPath / "resources");
        }
        modInfo.configuredEnabled = isEnabled(modInfo.id, modInfo.enabledByDefault);
        if(const auto previousIt = previous.find(modInfo.id); previousIt != previous.end()) {
          modInfo.active = previousIt->second.active;
          if(modInfo.error.empty()) {
            modInfo.error = previousIt->second.error;
          }
        }
        if(!seenIds.insert(modInfo.id).second) {
          modInfo.id += "__duplicate__" + std::to_string(packageMods_.size());
          modInfo.error = "Duplicate mod id detected";
          modInfo.configuredEnabled = false;
        }
        packageMods_.push_back(std::move(modInfo));
        it.disable_recursion_pending();
        continue;
      }
      if(!entry.is_regular_file() || filename == "mod-state.cfg" || toLowerCopy(path.extension().string()) != ".zip") {
        continue;
      }
      ModPackage modInfo;
      bool discovered = false;
      std::vector<ZipEntry> zipEntries;
      std::error_code archiveSizeError;
      const std::uintmax_t archiveSize = std::filesystem::file_size(path, archiveSizeError);
      std::vector<std::uint8_t> archive;
      if(archiveSizeError || archiveSize > kMaxModArchiveBytes) {
        modInfo = makeBrokenPackage(ModPackageSource::Zip, path, sanitizeName(path.stem().string()),
                                    "Mod archive is too large");
      } else {
        archive = readFileBytes(path);
      }
      if(modInfo.id.empty() && !buildZipIndex(archive, zipEntries)) {
        modInfo =
            makeBrokenPackage(ModPackageSource::Zip, path, sanitizeName(path.stem().string()), "Unable to read zip");
      } else if(modInfo.id.empty()) {
        const ZipEntry* manifestEntry = findZipEntry(zipEntries, "mod.json");
        if(manifestEntry == nullptr) {
          modInfo = makeBrokenPackage(ModPackageSource::Zip, path, sanitizeName(path.stem().string()),
                                      "Zip package is missing mod.json");
        } else {
          const std::vector<std::uint8_t> manifestBytes = readZipEntryData(archive, *manifestEntry);
          const std::string manifestText(manifestBytes.begin(), manifestBytes.end());
          discovered = parseManifestJson(manifestText, modInfo, path, ModPackageSource::Zip, "mod.json: ");
          if(discovered) {
            std::error_code ec;
            const auto stamp = std::filesystem::last_write_time(path, ec).time_since_epoch().count();
            const std::string cacheName = sanitizeName(path.stem().string()) + "_" + std::to_string(archiveSize) +
                                          "_" + std::to_string(stamp);
            modInfo.rootPath = cacheDirectory_ / cacheName;
            std::filesystem::remove_all(modInfo.rootPath, ec);
            std::filesystem::create_directories(modInfo.rootPath);
            for(const ZipEntry& zipEntry : zipEntries) {
              const std::string relativePath = normalizeRelativePath(zipEntry.name);
              if(relativePath.empty() || isDirectoryZipPath(relativePath) || !isSafeRelativePath(relativePath)) {
                continue;
              }
              const std::filesystem::path outPath = modInfo.rootPath / std::filesystem::path(relativePath);
              (void)writeFileBytes(outPath, readZipEntryData(archive, zipEntry));
            }
            modInfo.resourceOverlay = std::filesystem::is_directory(modInfo.rootPath / "resources");
          }
        }
      }
      if(modInfo.id.empty()) {
        continue;
      }
      modInfo.configuredEnabled = isEnabled(modInfo.id, modInfo.enabledByDefault);
      if(const auto previousIt = previous.find(modInfo.id); previousIt != previous.end()) {
        modInfo.active = previousIt->second.active;
        if(modInfo.error.empty()) {
          modInfo.error = previousIt->second.error;
        }
      }
      if(!seenIds.insert(modInfo.id).second) {
        modInfo.id += "__duplicate__" + std::to_string(packageMods_.size());
        modInfo.error = "Duplicate mod id detected";
        modInfo.configuredEnabled = false;
      }
      packageMods_.push_back(std::move(modInfo));
    }
  }
  sortMods(packageMods_);
}
void ModHost::loadEnabledPackageMods() {
  if(packageModsLoaded_) {
    return;
  }
  packageModsLoaded_ = true;
  for(ModPackage& mod : packageMods_) {
    mod.active = false;
    if(!mod.configuredEnabled) {
      continue;
    }
    (void)loadLuaMod(mod, loadedLuaMods_);
  }
}
std::vector<ModPackage> ModHost::packageMods() const {
  return packageMods_;
}
std::vector<std::uint8_t> ModHost::readResource(std::string_view path) const {
  const std::string normalized = normalizeRelativePath(path);
  if(normalized.empty() || !isSafeRelativePath(normalized)) {
    return {};
  }
  for(auto it = packageMods_.rbegin(); it != packageMods_.rend(); ++it) {
    if(!it->active || !it->resourceOverlay) {
      continue;
    }
    const std::filesystem::path candidate = it->rootPath / "resources" / std::filesystem::path(normalized);
    if(std::filesystem::is_regular_file(candidate)) {
      return readFileBytes(candidate);
    }
  }
  return {};
}
std::filesystem::path ModHost::modsDirectory() const {
  return modsDirectory_;
}
std::filesystem::path ModHost::runDirectory() const {
  return runDirectory_;
}
std::filesystem::path ModHost::assetPath(std::string_view modId, std::string_view relativePath) const {
  if(!isSafeRelativePath(relativePath)) {
    return {};
  }
  const ModPackage* mod = findPackageMod(std::string(modId));
  if(mod == nullptr || mod->rootPath.empty()) {
    return {};
  }
  const std::filesystem::path candidate = mod->rootPath / std::filesystem::path(normalizeRelativePath(relativePath));
  if(!std::filesystem::exists(candidate)) {
    return {};
  }
  return candidate;
}
} // namespace net::minecraft::mod::runtime
