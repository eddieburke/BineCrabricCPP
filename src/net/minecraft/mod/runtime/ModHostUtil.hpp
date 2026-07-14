#pragma once
#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>
namespace net::minecraft {
class World;
namespace entity::player {
class PlayerEntity;
}
} // namespace net::minecraft
struct lua_State;
namespace net::minecraft::mod::runtime {
inline constexpr std::uintmax_t kMaxModArchiveBytes = 256U * 1024U * 1024U;
inline constexpr std::uint64_t kMaxModEntryBytes = 64U * 1024U * 1024U;
inline constexpr std::uint64_t kMaxModExtractedBytes = 512U * 1024U * 1024U;
inline constexpr std::uint16_t kMaxModArchiveEntries = 4096;
// Returns the world the currently-running mod code is scoped to (set by
// setModContext / ModContextScope on both the server tick/interact paths and the
// client tick path). When no scope is active the call falls back to the CLIENT
// replica world (Minecraft::INSTANCE->world) — this only happens for unscoped
// client-side Lua such as render/HUD/screen callbacks. Server entry points always
// establish a scope, so server Lua never reaches the fallback. Mutating callers
// must still gate on !world->isRemote(); the name makes that fallback visible.
[[nodiscard]] World* contextOrClientWorld();
class ModContextScope {
public:
  ModContextScope(World* world, entity::player::PlayerEntity* player = nullptr);
  ~ModContextScope();
  ModContextScope(const ModContextScope&) = delete;
  ModContextScope& operator=(const ModContextScope&) = delete;

private:
  World* previousWorld_ = nullptr;
  bool previousIsClient_ = true;
  entity::player::PlayerEntity* previousPlayer_ = nullptr;
};
void setModContext(World* world, bool isClient, entity::player::PlayerEntity* player = nullptr);
void clearModContext();
[[nodiscard]] bool modContextIsClient();
[[nodiscard]] entity::player::PlayerEntity* activeModPlayer();
std::string trimCopy(std::string value);
std::string toLowerCopy(std::string value);
std::string normalizeRelativePath(std::string_view value);
bool isSafeRelativePath(std::string_view value);
bool isSafeModId(std::string_view value);
bool isDirectoryZipPath(std::string_view value);
std::string sanitizeName(std::string_view value);
std::vector<std::uint8_t> readFileBytes(const std::filesystem::path& path);
std::string readFileText(const std::filesystem::path& path);
bool writeFileBytes(const std::filesystem::path& path, const std::vector<std::uint8_t>& bytes);
bool writeFileText(const std::filesystem::path& path, const std::string& text);
} // namespace net::minecraft::mod::runtime
