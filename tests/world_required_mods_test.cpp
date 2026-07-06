#include "net/minecraft/mod/runtime/WorldRequiredMods.hpp"
#include <gtest/gtest.h>
#include <filesystem>
namespace {
using net::minecraft::mod::runtime::WorldRequiredMods;
TEST(WorldRequiredMods, CsvRoundTrip) {
  const std::vector<std::string> mods{"iron_bars", "stone_bricks"};
  const std::string csv = WorldRequiredMods::joinCsv(mods);
  EXPECT_EQ(csv, "iron_bars,stone_bricks");
  EXPECT_EQ(WorldRequiredMods::splitCsv(csv), mods);
  EXPECT_TRUE(WorldRequiredMods::splitCsv("").empty());
  EXPECT_EQ(WorldRequiredMods::splitCsv(" a , ,b ").size(), 2U);
}
TEST(WorldRequiredMods, FileRoundTrip) {
  const std::filesystem::path dir = std::filesystem::temp_directory_path() / "wrm_test_world";
  std::filesystem::remove_all(dir);
  std::filesystem::create_directories(dir);
  EXPECT_TRUE(WorldRequiredMods::readWorldFile(dir).empty());
  WorldRequiredMods::registerContentBlock("test_mod", 200);
  EXPECT_TRUE(WorldRequiredMods::isModBlockId(200));
  EXPECT_FALSE(WorldRequiredMods::isModBlockId(1));
  const auto* fakeWorld = reinterpret_cast<const net::minecraft::World*>(&dir);
  WorldRequiredMods::notePlaced(fakeWorld, 200);
  WorldRequiredMods::notePlaced(fakeWorld, 1);
  EXPECT_EQ(WorldRequiredMods::sessionMods(fakeWorld), std::vector<std::string>{"test_mod"});
  WorldRequiredMods::writeWorldFile(dir, fakeWorld);
  EXPECT_EQ(WorldRequiredMods::readWorldFile(dir), std::vector<std::string>{"test_mod"});
  WorldRequiredMods::forgetWorld(fakeWorld);
  EXPECT_TRUE(WorldRequiredMods::sessionMods(fakeWorld).empty());
  EXPECT_EQ(WorldRequiredMods::readWorldFile(dir), std::vector<std::string>{"test_mod"});
  std::filesystem::remove_all(dir);
}
TEST(WorldRequiredMods, MissingModsAgainstHost) {
  const std::vector<std::string> required{"definitely_not_installed_mod"};
  const std::vector<std::string> missing = WorldRequiredMods::missingMods(required);
  EXPECT_EQ(missing, required);
}
} // namespace
