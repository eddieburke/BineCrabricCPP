#pragma once
#include <filesystem>
#include <string>
#include <vector>
namespace net::minecraft {
class World;
}
namespace net::minecraft::mod::runtime {
class WorldRequiredMods {
public:
  static void registerContentBlock(const std::string& modId, int blockId);
  static bool isModBlockId(int blockId) noexcept;
  static void notePlaced(const World* world, int blockId);
  static void forgetWorld(const World* world);
  static std::vector<std::string> sessionMods(const World* world);
  static std::vector<std::string> readWorldFile(const std::filesystem::path& worldDirectory);
  static void writeWorldFile(const std::filesystem::path& worldDirectory, const World* world);
  static std::vector<std::string> requiredForDirectory(const std::filesystem::path& worldDirectory);
  static std::vector<std::string> missingMods(const std::vector<std::string>& required);
  static std::string joinCsv(const std::vector<std::string>& mods);
  static std::vector<std::string> splitCsv(const std::string& csv);
};
} // namespace net::minecraft::mod::runtime
