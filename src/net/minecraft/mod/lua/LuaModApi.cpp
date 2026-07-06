#include "net/minecraft/mod/lua/LuaHostApi.hpp"
#include "net/minecraft/mod/lua/LuaJsonApi.hpp"
#include "net/minecraft/mod/runtime/ModHost.hpp"
#include "net/minecraft/util/SeedText.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
#include "net/minecraft/world/biome/Biome.hpp"
#include "net/minecraft/world/biome/BiomeNames.hpp"
#include "net/minecraft/world/biome/source/BiomeSource.hpp"
#include "net/minecraft/world/chunk/Chunk.hpp"
#include "net/minecraft/world/gen/chunk/OverworldChunkGenerator.hpp"
#ifdef MINECRAFT_NATIVE_EXPORTS
#include "net/minecraft/client/platform/FileDialog.hpp"
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/texture/TextureManager.hpp"
#endif
#include <algorithm>
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
[[nodiscard]] int floorDiv16(int value) {
  return MathHelper::floorDiv(value, 16);
}
[[nodiscard]] int mod16(int value) {
  return MathHelper::floorMod(value, 16);
}
[[nodiscard]] Chunk& terrainChunk(OverworldChunkGenerator& generator,
                                  std::unordered_map<std::uint64_t, Chunk>& chunks, int worldX, int worldZ) {
  const int chunkX = floorDiv16(worldX);
  const int chunkZ = floorDiv16(worldZ);
  const std::uint64_t key = (static_cast<std::uint64_t>(static_cast<std::uint32_t>(chunkX)) << 32U) |
                            static_cast<std::uint32_t>(chunkZ);
  auto found = chunks.find(key);
  if(found == chunks.end()) {
    Chunk chunk = generator.loadChunk(nullptr, chunkX, chunkZ);
    found = chunks.emplace(key, std::move(chunk)).first;
  }
  return found->second;
}
[[nodiscard]] std::string normalizeGridChannel(std::string channel) {
  if(channel == "biome_grass") {
    return "grass";
  }
  if(channel == "surface_y") {
    return "height";
  }
  if(channel == "top_block") {
    return "surface_block";
  }
  if(channel == "block_below_surface") {
    return "surface_block_below";
  }
  return channel;
}
[[nodiscard]] bool isTerrainGridChannel(const std::string& channel) {
  return channel == "height" || channel == "surface_block" || channel == "surface_block_below";
}
[[nodiscard]] long long sampleTerrainChannel(OverworldChunkGenerator& generator,
                                             std::unordered_map<std::uint64_t, Chunk>& chunks,
                                             const std::string& channel, int worldX, int worldZ) {
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
  api.createtable(state, static_cast<int>(names.size()), 0);
  for(std::size_t i = 0; i < names.size(); ++i) {
    api.pushstring(state, names[i].c_str());
    api.rawseti(state, -2, static_cast<long long>(i + 1));
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
  const int centerX =
      isNumber != 0 ? static_cast<int>(std::clamp(requestedCenterX, -30'000'000LL, 30'000'000LL)) : 0;
  isNumber = 0;
  const long long requestedCenterZ = api.tointegerx(state, 3, &isNumber);
  const int centerZ =
      isNumber != 0 ? static_cast<int>(std::clamp(requestedCenterZ, -30'000'000LL, 30'000'000LL)) : 0;
  int radiusChunks = 6;
  int maxSide = 48;
  std::string channel = "grass";
  std::vector<std::string> channels;
  bool modGeneration = false;
  if(api.type(state, 4) == kLuaTTable) {
    radiusChunks = std::clamp(
        luaIntField(state, 4, "radius_chunks", luaIntField(state, 4, "radius", radiusChunks)), 1, 4096);
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
          const std::string requested = normalizeGridChannel(luaString(state, -1, ""));
          if(!requested.empty() && std::find(channels.begin(), channels.end(), requested) == channels.end()) {
            channels.push_back(requested);
          }
        }
        api.settop(state, -2);
      }
    }
    api.settop(state, -2);
  }
  channel = normalizeGridChannel(channel);
  if(channels.empty()) {
    channels.push_back(channel);
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
  const bool needsTerrain =
      std::any_of(channels.begin(), channels.end(), [](const std::string& name) { return isTerrainGridChannel(name); });
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
          cell = static_cast<long long>(0xFF000000U | static_cast<std::uint32_t>(biome->grassColor & 0xFFFFFF));
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
  const int textureId = client::Minecraft::INSTANCE->textureManager.load(image);
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
  client::Minecraft::INSTANCE->textureManager.deleteTexture(textureId);
  mod->ownedTextureIds.erase(it);
  api.pushboolean(state, 1);
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
  bindFunctions(state, {{"_resolve_seed", luaResolveSeed}, {"_json_encode", luaJsonEncode}, {"_json_decode", luaJsonDecode}});
  pushFunctionTable(state, {
                               {"name", luaRegistryName},
                               {"list", luaRegistryList},
                           });
  api.setfield(state, root, "registry");
  api.getfield(state, root, "world");
  bindFunctions(state, {{"sample_grid", luaWorldSampleGrid}});
  api.settop(state, root);
#ifdef MINECRAFT_NATIVE_EXPORTS
  pushFunctionTable(state, {{"pick", luaFilesPick}, {"read", luaFilesRead}});
  api.setfield(state, root, "files");
  api.getfield(state, root, "render");
  bindModFunction(state, &mod, "create_texture", luaRenderCreateTexture);
  bindModFunction(state, &mod, "release_texture", luaRenderReleaseTexture);
  api.settop(state, root);
#endif
}
} // namespace net::minecraft::mod::lua
