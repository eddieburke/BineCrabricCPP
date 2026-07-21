#include "net/minecraft/mod/lua/LuaGameApi.hpp"
#include "net/minecraft/mod/lua/LuaHostApi.hpp"
#include "net/minecraft/mod/lua/LuaJsonApi.hpp"
#include "net/minecraft/mod/lua/LuaChunkContext.hpp"
#include "net/minecraft/mod/runtime/ModHost.hpp"
#include "net/minecraft/mod/lua/LuaHostApi.hpp"
#include "net/minecraft/util/SeedText.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/world/biome/Biome.hpp"
#include "net/minecraft/world/biome/BiomeNames.hpp"
#include "net/minecraft/world/biome/source/BiomeSource.hpp"
#include "net/minecraft/world/chunk/Chunk.hpp"
#include "net/minecraft/world/gen/chunk/OverworldChunkGenerator.hpp"
#ifdef MINECRAFT_NATIVE_EXPORTS
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/platform/FileDialog.hpp"
#include "net/minecraft/client/texture/TextureManager.hpp"
#include "net/minecraft/client/gl/GLCore.hpp"
#include "net/minecraft/client/gl/GlConstants.hpp"
#include "net/minecraft/client/render/RenderSystem.hpp"
#include "net/minecraft/registry/TextureRegistry.hpp"
#include "net/minecraft/client/ClientLog.hpp"
#endif
#include <algorithm>
#include <atomic>
#include <fstream>
#include <memory>
#include <sstream>
#include <unordered_map>
#include <vector>
namespace net::minecraft::mod::lua {
namespace {
using LoadedLuaMod = net::minecraft::mod::runtime::ModHost::LoadedLuaMod;
LoadedLuaMod* modFromUpvalue(lua_State* state) {
 return static_cast<LoadedLuaMod*>(luaApi().touserdata(state, luaUpvalueIndex(1)));
}
#ifdef MINECRAFT_NATIVE_EXPORTS
void warnLegacyRawTextureId(int textureId) {
 static std::unordered_map<int, bool> warned;
 if(warned.emplace(textureId, true).second) {
  net::minecraft::client::ClientLog::LOGGER.log(
      net::minecraft::util::logging::LogLevel::Warning,
      "Lua mod passed a raw GL texture id (" + std::to_string(textureId) +
          ") to a texture API; this is deprecated, use the id returned by create_texture directly");
 }
}
int resolveLuaTextureGlId(int textureId) {
 if(net::minecraft::registry::TextureRegistry::isCustomTexture(textureId)) {
  if(client::Minecraft::INSTANCE == nullptr) {
   return -1;
  }
  return net::minecraft::registry::TextureRegistry::resolveGlId(textureId, client::Minecraft::INSTANCE->textureManager);
 }
 warnLegacyRawTextureId(textureId);
 return textureId;
}
#endif
[[nodiscard]] int floorDiv16(int value) {
 return MathHelper::floorDiv(value, 16);
}
[[nodiscard]] int mod16(int value) {
 return MathHelper::floorMod(value, 16);
}
[[nodiscard]] Chunk& terrainChunk(OverworldChunkGenerator& generator,
                                  std::unordered_map<std::uint64_t, Chunk>& chunks,
                                  int worldX,
                                  int worldZ) {
 const int chunkX = floorDiv16(worldX);
 const int chunkZ = floorDiv16(worldZ);
 const std::uint64_t key =
     (static_cast<std::uint64_t>(static_cast<std::uint32_t>(chunkX)) << 32U) | static_cast<std::uint32_t>(chunkZ);
 auto found = chunks.find(key);
 if(found == chunks.end()) {
  Chunk chunk = generator.loadChunk(nullptr, chunkX, chunkZ);
  found = chunks.emplace(key, std::move(chunk)).first;
 }
 return found->second;
}
[[nodiscard]] bool isTerrainGridChannel(const std::string& channel) {
 return channel == "height" || channel == "surface_block" || channel == "surface_block_below";
}
[[nodiscard]] bool isSupportedSampleChannel(const std::string& channel) {
 return isTerrainGridChannel(channel) || channel == "biome_id" || channel == "grass";
}
[[nodiscard]] const std::array<const char*, 5>& sampleChannels() {
 static const std::array<const char*, 5> channels = {
     "height", "surface_block", "surface_block_below", "biome_id", "grass"};
 return channels;
}
[[nodiscard]] long long sampleTerrainChannel(OverworldChunkGenerator& generator,
                                             std::unordered_map<std::uint64_t, Chunk>& chunks,
                                             const std::string& channel,
                                             int worldX,
                                             int worldZ) {
 Chunk& chunk = terrainChunk(generator, chunks, worldX, worldZ);
 const int localX = mod16(worldX);
 const int localZ = mod16(worldZ);
 const int height = chunk.getHeight(localX, localZ);
 if(channel == "height") {
  return height;
 }
 const int offset = channel == "surface_block_below" ? 2 : 1;
 const int y = std::clamp(height - offset, 0, Chunk::height - 1);
 return chunk.getBlockId(localX, y, localZ);
}
int luaRegistryName(lua_State* state) {
 LuaApi& api = luaApi();
 const std::string domain = luaString(state, 1, "");
 int isNumber = 0;
 const int id = static_cast<int>(api.tointegerx(state, 2, &isNumber));
 if(domain == "biome") {
  api.pushstring(state, net::minecraft::biomeWireName(id).c_str());
  return 1;
 }
#ifdef MINECRAFT_NATIVE_EXPORTS
 if(domain == "block") {
  const std::string wire = blockWireNameFromId(id);
  api.pushstring(state, wire.empty() ? "unknown" : wire.c_str());
  return 1;
 }
#endif
 api.pushstring(state, "unknown");
 return 1;
}
int luaRegistryList(lua_State* state) {
 LuaApi& api = luaApi();
 const std::string domain = luaString(state, 1, "");
 std::vector<std::string> names;
 if(domain == "biome") {
  names = net::minecraft::allBiomeWireNames();
 }
#ifdef MINECRAFT_NATIVE_EXPORTS
 if(domain == "block") {
  names.reserve(128);
  for(int blockId = 1; blockId < block::Block::BLOCK_COUNT; ++blockId) {
   if(block::Block::BLOCKS[static_cast<std::size_t>(blockId)] == nullptr) {
    continue;
   }
   const std::string wire = blockWireNameFromId(blockId);
   if(!wire.empty()) {
    names.push_back(wire);
   }
  }
 }
#endif
 api.createtable(state, static_cast<int>(names.size()), 0);
 for(std::size_t i = 0; i < names.size(); ++i) {
  api.pushstring(state, names[i].c_str());
  api.rawseti(state, -2, static_cast<long long>(i + 1));
 }
 return 1;
}
int luaWorldGetBlockCollisions(lua_State* state) {
 LuaApi& api = luaApi();
 if(api.type(state, 1) != kLuaTTable) {
  api.pushnil(state);
  return 1;
 }
 const double minX = luaFloatField(state, 1, "min_x", 0.0f);
 const double minY = luaFloatField(state, 1, "min_y", 0.0f);
 const double minZ = luaFloatField(state, 1, "min_z", 0.0f);
 const double maxX = luaFloatField(state, 1, "max_x", 0.0f);
 const double maxY = luaFloatField(state, 1, "max_y", 0.0f);
 const double maxZ = luaFloatField(state, 1, "max_z", 0.0f);
 World* world = LuaChunkContext::hasActiveChunk() ? LuaChunkContext::activeWorld() : nullptr;
 if(world == nullptr) {
#ifdef MINECRAFT_NATIVE_EXPORTS
  world = client::Minecraft::INSTANCE != nullptr ? client::Minecraft::INSTANCE->world : nullptr;
#endif
 }
 if(world == nullptr) {
  api.pushnil(state);
  return 1;
 }
 net::minecraft::Box aabb{minX, minY, minZ, maxX, maxY, maxZ};
 std::vector<net::minecraft::Box> collisions = world->getBlockCollisions(aabb);
 api.createtable(state, static_cast<int>(collisions.size()), 0);
 for(std::size_t i = 0; i < collisions.size(); ++i) {
  const auto& box = collisions[i];
  api.createtable(state, 0, 6);
  setField(state, "min_x", box.minX);
  setField(state, "min_y", box.minY);
  setField(state, "min_z", box.minZ);
  setField(state, "max_x", box.maxX);
  setField(state, "max_y", box.maxY);
  setField(state, "max_z", box.maxZ);
  api.rawseti(state, -2, static_cast<int>(i + 1));
 }
 return 1;
}
int luaWorldSampleGrid(lua_State* state) {
 LuaApi& api = luaApi();
 int isNumber = 0;
 const long long signedSeed = api.tointegerx(state, 1, &isNumber);
 const std::uint64_t seed = isNumber != 0 ? static_cast<std::uint64_t>(signedSeed) : 0;
 isNumber = 0;
 const long long requestedCenterX = api.tointegerx(state, 2, &isNumber);
 const int centerX = isNumber != 0 ? static_cast<int>(std::clamp(requestedCenterX, -30'000'000LL, 30'000'000LL)) : 0;
 isNumber = 0;
 const long long requestedCenterZ = api.tointegerx(state, 3, &isNumber);
 const int centerZ = isNumber != 0 ? static_cast<int>(std::clamp(requestedCenterZ, -30'000'000LL, 30'000'000LL)) : 0;
 int radiusChunks = 6;
 int maxSide = 48;
 std::string channel = "grass";
 std::vector<std::string> channels;
 bool modGeneration = false;
 if(api.type(state, 4) == kLuaTTable) {
  radiusChunks =
      std::clamp(luaIntField(state, 4, "radius_chunks", luaIntField(state, 4, "radius", radiusChunks)), 1, 4096);
  maxSide = std::clamp(luaIntField(state, 4, "max_side", maxSide), 8, 256);
  channel = luaStringField(state, 4, "channel", channel);
  modGeneration = luaBoolField(state, 4, "mod_generation", false);
  api.getfield(state, 4, "channels");
  if(api.type(state, -1) == kLuaTTable) {
   const int channelTable = api.gettop(state);
   const std::size_t count = std::min<std::size_t>(api.rawlen(state, channelTable), 8);
   for(std::size_t i = 1; i <= count; ++i) {
    api.rawgeti(state, channelTable, static_cast<long long>(i));
    if(api.type(state, -1) == kLuaTString) {
     const std::string requested = luaString(state, -1, "");
     if(!requested.empty() &&
        std::find(channels.begin(), channels.end(), requested) == channels.end()) {
      channels.push_back(requested);
     }
    }
    api.settop(state, -2);
   }
  }
  api.settop(state, -2);
 }
 if(channels.empty()) {
  channels.push_back(channel);
 }
 for(const std::string& requested : channels) {
  if(!isSupportedSampleChannel(requested)) {
   api.pushnil(state);
   api.pushstring(state, ("unsupported sample channel: " + requested).c_str());
   return 2;
  }
 }
 channel = channels.front();
 radiusChunks = std::clamp(radiusChunks, 1, 4096);
 maxSide = std::clamp(maxSide, 8, 256);
 const int radiusBlocks = std::max(16, radiusChunks * 16);
 const int blocksPerSide = radiusBlocks * 2;
 const int side = std::min(maxSide, blocksPerSide);
 const int step = std::max(1, blocksPerSide / side);
 const int startX = centerX - radiusBlocks;
 const int startZ = centerZ - radiusBlocks;
 net::minecraft::Biome::init();
 net::minecraft::BiomeSource source(seed);
 std::unique_ptr<OverworldChunkGenerator> generator;
 std::unordered_map<std::uint64_t, Chunk> terrainChunks;
 const bool needsTerrain = std::any_of(
     channels.begin(), channels.end(), [](const std::string& name) { return isTerrainGridChannel(name); });
 const bool needsBiome = std::any_of(channels.begin(), channels.end(), [](const std::string& name) {
  return name == "biome_id" || name == "grass";
 });
 if(needsTerrain) {
  generator = std::make_unique<OverworldChunkGenerator>(nullptr, seed);
  generator->useLocalBiomeSource(true);
  generator->enableModGeneration(modGeneration);
 }
 api.createtable(state, 0, 8);
 setField(state, "side", side);
 setField(state, "step", step);
 setField(state, "origin_x", startX);
 setField(state, "origin_z", startZ);
 setField(state, "center_x", centerX);
 setField(state, "center_z", centerZ);
 setField(state, "channel", channel);
 std::vector<std::vector<long long>> samples(channels.size());
 for(auto& values : samples) {
  values.reserve(static_cast<std::size_t>(side * side));
 }
 for(int row = 0; row < side; ++row) {
  for(int col = 0; col < side; ++col) {
   const int sampleX = std::min(blocksPerSide - 1, col * step);
   const int sampleZ = std::min(blocksPerSide - 1, row * step);
   const int worldX = startX + sampleX;
   const int worldZ = startZ + sampleZ;
   Biome* biome = needsBiome ? &source.getBiome(worldX, worldZ) : nullptr;
   for(std::size_t channelIndex = 0; channelIndex < channels.size(); ++channelIndex) {
    const std::string& sampleChannel = channels[channelIndex];
    long long cell = 0;
    if(isTerrainGridChannel(sampleChannel) && generator != nullptr) {
     cell = sampleTerrainChannel(*generator, terrainChunks, sampleChannel, worldX, worldZ);
    } else if(sampleChannel == "biome_id" && biome != nullptr) {
     cell = static_cast<long long>(static_cast<int>(biome->id));
    } else if(sampleChannel == "grass" && biome != nullptr) {
     cell =
         static_cast<long long>(0xFF000000U | static_cast<std::uint32_t>(biome->grassColor & 0xFFFFFF));
    }
    samples[channelIndex].push_back(cell);
   }
  }
 }
 const auto pushSamples = [&api, state](const std::vector<long long>& values) {
  api.createtable(state, static_cast<int>(values.size()), 0);
  for(std::size_t i = 0; i < values.size(); ++i) {
   api.pushinteger(state, values[i]);
   api.rawseti(state, -2, static_cast<long long>(i + 1));
  }
 };
 pushSamples(samples.front());
 api.setfield(state, -2, "values");
 api.createtable(state, 0, static_cast<int>(channels.size()));
 for(std::size_t i = 0; i < channels.size(); ++i) {
  pushSamples(samples[i]);
  api.setfield(state, -2, channels[i].c_str());
 }
 api.setfield(state, -2, "channels");
 return 1;
}
int luaWorldSampleChannels(lua_State* state) {
 LuaApi& api = luaApi();
 const auto& channels = sampleChannels();
 api.createtable(state, static_cast<int>(channels.size()), 0);
 for(std::size_t i = 0; i < channels.size(); ++i) {
  api.pushstring(state, channels[i]);
  api.rawseti(state, -2, static_cast<long long>(i + 1));
 }
 return 1;
}
#ifdef MINECRAFT_NATIVE_EXPORTS
int luaFilesPick(lua_State* state) {
 LuaApi& api = luaApi();
 std::string extension;
 if(api.type(state, 1) == kLuaTTable) {
  extension = luaStringField(state, 1, "extension", "");
  if(extension.empty()) {
   extension = luaStringField(state, 1, "filter", "");
  }
 } else if(api.type(state, 1) == kLuaTString) {
  extension = luaString(state, 1, "");
 }
 std::optional<std::filesystem::path> picked;
 if(extension.empty() || extension == "json" || extension == ".json" || extension == "*.json") {
  picked = client::platform::pickJsonFile();
 } else {
  std::string label = extension;
  std::string pattern = extension;
  if(pattern.front() != '*') {
   if(pattern.front() != '.') {
    pattern.insert(pattern.begin(), '.');
   }
   pattern.insert(pattern.begin(), '*');
  }
  if(label.front() == '*') {
   label.erase(label.begin());
  }
  if(!label.empty() && label.front() == '.') {
   label.erase(label.begin());
  }
  label.append(" files");
  picked = client::platform::pickFile(label, pattern);
 }
 if(picked.has_value()) {
  api.pushstring(state, picked->string().c_str());
 } else {
  api.pushnil(state);
 }
 return 1;
}
int luaFilesRead(lua_State* state) {
 LuaApi& api = luaApi();
 const std::string path = luaString(state, 1, "");
 // Mod-bundled resources are addressed relative to the mod's root (e.g.
 // "resources/mods/<id>/foo.json"); ModHost::findResourceFile resolves that
 // against the mod's actual extraction/cache directory rather than the
 // process's current working directory. Using findResourceFile (which
 // returns an optional path) rather than readResource (which returns an
 // empty vector both when the resource is missing AND when it's a
 // legitimately empty file) keeps "not found" and "found but empty"
 // distinguishable.
 std::string resourcePath = path;
 constexpr std::string_view kResourcesPrefix = "resources/";
 if(resourcePath.rfind(kResourcesPrefix, 0) == 0) {
  resourcePath.erase(0, kResourcesPrefix.size());
 }
 if(const std::optional<std::filesystem::path> resolved =
        net::minecraft::mod::runtime::host().resolveResourcePath(resourcePath);
    resolved.has_value()) {
  const std::vector<std::uint8_t> resourceBytes = net::minecraft::mod::lua::readFileBytes(*resolved);
  const std::string text(resourceBytes.begin(), resourceBytes.end());
  api.pushstring(state, text.c_str());
  return 1;
 }
 std::ifstream input(path, std::ios::binary);
 if(!input) {
  api.pushnil(state);
  api.pushstring(state, "failed to read file");
  return 2;
 }
 std::ostringstream buffer;
 buffer << input.rdbuf();
 const std::string text = buffer.str();
 if(text.empty()) {
  api.pushnil(state);
  api.pushstring(state, "file is empty");
  return 2;
 }
 api.pushstring(state, text.c_str());
 return 1;
}
int luaRenderCreateTexture(lua_State* state) {
 LuaApi& api = luaApi();
 LoadedLuaMod* mod = modFromUpvalue(state);
 if(mod == nullptr || client::Minecraft::INSTANCE == nullptr) {
  api.pushnil(state);
  return 1;
 }
 int width = 1;
 int height = 1;
 int colorsIndex = 0;
 if(api.type(state, 1) == kLuaTTable) {
  width = std::max(1, luaIntField(state, 1, "width", luaIntField(state, 1, "side", 1)));
  height = std::max(1, luaIntField(state, 1, "height", width));
  api.getfield(state, 1, "values");
  if(api.type(state, -1) != kLuaTTable) {
   api.getfield(state, 1, "colors");
  }
  colorsIndex = api.gettop(state);
 } else {
  int isNumber = 0;
  width = std::max(1, static_cast<int>(api.tointegerx(state, 1, &isNumber)));
  height = std::max(1, static_cast<int>(api.tointegerx(state, 2, &isNumber)));
  colorsIndex = 3;
 }
 if(api.type(state, colorsIndex) != kLuaTTable) {
  api.settop(state, -2);
  api.pushnil(state);
  return 1;
 }
 const int cellCount = width * height;
 client::texture::RasterImage image{};
 image.width = width;
 image.height = height;
 image.argb.assign(static_cast<std::size_t>(cellCount), 0xFF000000U);
 for(int i = 1; i <= cellCount; ++i) {
  api.rawgeti(state, colorsIndex, static_cast<long long>(i));
  int isNumber = 0;
  const long long argb = api.tointegerx(state, -1, &isNumber);
  api.settop(state, -2);
  if(isNumber != 0) {
   image.argb[static_cast<std::size_t>(i - 1)] = static_cast<std::uint32_t>(argb);
  }
 }
 api.settop(state, colorsIndex - 1);
 const int glId = client::Minecraft::INSTANCE->textureManager.load(image);
 static std::atomic<long long> nextModTextureSerial{0};
 const std::string syntheticPath =
     "mod://" + mod->modId + "/" + std::to_string(nextModTextureSerial.fetch_add(1) + 1);
 const int textureId = net::minecraft::registry::TextureRegistry::getOrRegisterTexture(syntheticPath);
 net::minecraft::registry::TextureRegistry::seedResolvedTexture(textureId, glId, width, height);
 mod->ownedTextureIds.push_back(textureId);
 api.createtable(state, 0, 3);
 setField(state, "id", textureId);
 setField(state, "width", width);
 setField(state, "height", height);
 return 1;
}
int luaRenderReleaseTexture(lua_State* state) {
 LuaApi& api = luaApi();
 LoadedLuaMod* mod = modFromUpvalue(state);
 int isNumber = 0;
 const int textureId = static_cast<int>(api.tointegerx(state, 1, &isNumber));
 if(mod == nullptr || isNumber == 0 || textureId <= 0) {
  api.pushboolean(state, 0);
  return 1;
 }
 const auto it = std::find(mod->ownedTextureIds.begin(), mod->ownedTextureIds.end(), textureId);
 if(it == mod->ownedTextureIds.end() || client::Minecraft::INSTANCE == nullptr) {
  api.pushboolean(state, 0);
  return 1;
 }
 const int glId = resolveLuaTextureGlId(textureId);
 if(glId >= 0) {
  client::Minecraft::INSTANCE->textureManager.deleteTexture(glId);
 }
 mod->ownedTextureIds.erase(it);
 api.pushboolean(state, 1);
 return 1;
}
int luaRenderUpdateTexture(lua_State* state) {
 LuaApi& api = luaApi();
 LoadedLuaMod* mod = modFromUpvalue(state);
 if(mod == nullptr || client::Minecraft::INSTANCE == nullptr) {
  api.pushboolean(state, 0);
  return 1;
 }
 int isNum = 0;
 const int textureId = static_cast<int>(api.tointegerx(state, 1, &isNum));
 if(isNum == 0 || textureId <= 0) {
  api.pushboolean(state, 0);
  return 1;
 }
 const auto it = std::find(mod->ownedTextureIds.begin(), mod->ownedTextureIds.end(), textureId);
 if(it == mod->ownedTextureIds.end()) {
  api.pushboolean(state, 0);
  return 1;
 }
 const int glId = resolveLuaTextureGlId(textureId);
 const auto* cached = client::Minecraft::INSTANCE->textureManager.getRasterImage(glId);
 if(cached == nullptr) {
  api.pushboolean(state, 0);
  return 1;
 }
 int width = cached->width;
 int height = cached->height;
 int colorsIndex = 2;
 if(api.type(state, 2) == kLuaTTable) {
  api.getfield(state, 2, "values");
  if(api.type(state, -1) != kLuaTTable) {
   api.settop(state, -2);
   api.getfield(state, 2, "colors");
  }
  if(api.type(state, -1) == kLuaTTable) {
   width = std::max(1, luaIntField(state, 2, "width", luaIntField(state, 2, "side", width)));
   height = std::max(1, luaIntField(state, 2, "height", width));
   colorsIndex = api.gettop(state);
  } else {
   api.settop(state, -2);
   colorsIndex = 2;
  }
 }
 if(api.type(state, colorsIndex) != kLuaTTable) {
  api.pushboolean(state, 0);
  return 1;
 }
 const int cellCount = width * height;
 client::texture::RasterImage image{};
 image.width = width;
 image.height = height;
 image.argb.assign(static_cast<std::size_t>(cellCount), 0xFF000000U);
 for(int i = 1; i <= cellCount; ++i) {
  api.rawgeti(state, colorsIndex, static_cast<long long>(i));
  int isNumber = 0;
  const long long argb = api.tointegerx(state, -1, &isNumber);
  api.settop(state, -2);
  if(isNumber != 0) {
   image.argb[static_cast<std::size_t>(i - 1)] = static_cast<std::uint32_t>(argb);
  }
 }
 api.settop(state, colorsIndex - 1);
 client::Minecraft::INSTANCE->textureManager.update(glId, image);
 api.pushboolean(state, 1);
 return 1;
}
int luaRenderBindTexture(lua_State* state) {
 LuaApi& api = luaApi();
 int isNum = 0;
 const int textureId = static_cast<int>(api.tointegerx(state, 1, &isNum));
 if(isNum == 0 || textureId <= 0 || client::Minecraft::INSTANCE == nullptr) {
  api.pushboolean(state, 0);
  return 1;
 }
 const int glId = resolveLuaTextureGlId(textureId);
 if(glId < 0) {
  api.pushboolean(state, 0);
  return 1;
 }
 const int unit = static_cast<int>(api.tointegerx(state, 2, &isNum));
 if(isNum != 0) {
  int maxUnits = 0;
  client::render::RenderSystem::getIntegerv(client::gl::tex::MaxTextureImageUnits, &maxUnits);
  if(unit < 0 || unit >= maxUnits) {
   api.pushboolean(state, 0);
   return 1;
  }
  int previousActive = client::gl::tex::Texture0;
  client::render::RenderSystem::getIntegerv(client::gl::tex::ActiveTexture, &previousActive);
  client::render::RenderSystem::activeTexture(client::gl::tex::Texture0 + unit);
  client::render::RenderSystem::bindTexture(glId);
  client::render::RenderSystem::activeTexture(previousActive);
 } else {
  client::render::RenderSystem::bindTexture(glId);
 }
 api.pushboolean(state, 1);
 return 1;
}
int luaRenderGetTexturePixels(lua_State* state) {
 LuaApi& api = luaApi();
 if(api.gettop(state) < 1) {
  api.pushnil(state);
  return 1;
 }
 client::texture::RasterImage image;
 bool loaded = false;
 if(api.type(state, 1) == kLuaTString) {
  const std::string path = luaString(state, 1, "");
  if(!path.empty()) {
   image = client::Minecraft::INSTANCE->textureManager.loadRasterForResource(path);
   loaded = (image.width > 0 && image.height > 0);
  }
 } else if(api.type(state, 1) == kLuaTNumber) {
  const int textureId = static_cast<int>(api.tointegerx(state, 1, nullptr));
  const int glId = resolveLuaTextureGlId(textureId);
    const auto* entry = ::net::minecraft::registry::TextureRegistry::getEntry(textureId);
    if(entry != nullptr && !entry->path.empty() && entry->path.rfind("mod://", 0) != 0) {
     image = client::Minecraft::INSTANCE->textureManager.loadRasterForResource(entry->path);
     loaded = (image.width > 0 && image.height > 0);
    }
  if(!loaded && glId >= 0) {
   if(const auto* cached = client::Minecraft::INSTANCE->textureManager.getRasterImage(glId)) {
    image = *cached;
    loaded = true;
   }
  }
 }
 if(!loaded) {
  api.pushnil(state);
  return 1;
 }
 api.createtable(state, 0, 3);
 setField(state, "width", image.width);
 setField(state, "height", image.height);
 api.createtable(state, static_cast<int>(image.argb.size()), 0);
 for(std::size_t i = 0; i < image.argb.size(); ++i) {
  api.pushinteger(state, static_cast<long long>(image.argb[i]));
  api.rawseti(state, -2, static_cast<int>(i + 1));
 }
 api.setfield(state, -2, "pixels");
 return 1;
}
#endif
} // namespace
int luaResolveSeed(lua_State* state) {
 LuaApi& api = luaApi();
 const std::string text = luaString(state, 1, "");
 api.pushinteger(state, static_cast<std::int64_t>(net::minecraft::util::resolveSeedText(text)));
 return 1;
}
void installGenericModApi(lua_State* state, LoadedLuaMod& mod) {
 LuaApi& api = luaApi();
 const int root = api.gettop(state);
 pushFunctionTable(state,
                   {
                       {"resolve_seed", luaResolveSeed},
                       {"json_encode", luaJsonEncode},
                       {"json_decode", luaJsonDecode},
                   });
 api.pushlightuserdata(state, luaJsonNull());
 api.setfield(state, -2, "json_null");
 api.setfield(state, root, "util");
 pushFunctionTable(state,
                   {
                       {"name", luaRegistryName},
                       {"list", luaRegistryList},
                   });
 api.setfield(state, root, "registry");
 api.getfield(state, root, "world");
 bindFunctions(state,
               {{"sample", luaWorldSampleGrid},
                {"sample_grid", luaWorldSampleGrid},
                {"sample_channels", luaWorldSampleChannels},
                {"get_block_collisions", luaWorldGetBlockCollisions}});
 api.settop(state, root);
#ifdef MINECRAFT_NATIVE_EXPORTS
 pushFunctionTable(state, {{"pick", luaFilesPick}, {"read", luaFilesRead}});
 api.setfield(state, root, "files");
 api.getfield(state, root, "render");
 bindModFunction(state, &mod, "create_texture", luaRenderCreateTexture);
 bindModFunction(state, &mod, "update_texture", luaRenderUpdateTexture);
 bindModFunction(state, &mod, "release_texture", luaRenderReleaseTexture);
 bindModFunction(state, &mod, "get_texture_pixels", luaRenderGetTexturePixels);
 bindModFunction(state, &mod, "bind_texture", luaRenderBindTexture);
 api.setfield(state, -2, "render");
#endif
}
} // namespace net::minecraft::mod::lua
