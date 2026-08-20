// Kill-safety for the player's saved state. "End task" (or any hard kill / power loss) stops
// the process between saves, and can leave level.dat or a player .dat truncated or interrupt
// the atomic swap so only the "_old" backup survives. Losing the session's progress since the
// last save is expected; reloading with the player at world spawn holding nothing is not.
#include <gtest/gtest.h>
#include <atomic>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/entity/player/ServerPlayerEntity.hpp"
#include "net/minecraft/item/ItemStack.hpp"
#include "net/minecraft/nbt/NbtCompound.hpp"
#include "net/minecraft/nbt/NbtIo.hpp"
#include "net/minecraft/nbt/NbtList.hpp"
#include "net/minecraft/server/network/ServerPlayerInteractionManager.hpp"
#include "net/minecraft/world/WorldProperties.hpp"
#include "net/minecraft/world/storage/AlphaWorldStorage.hpp"
#include "support/server_event_fixture.hpp"
namespace fs = std::filesystem;
namespace {
using net::minecraft::AlphaWorldStorage;
using net::minecraft::ItemStack;
using net::minecraft::NbtCompound;
using net::minecraft::WorldProperties;
using net::minecraft::entity::player::ServerPlayerEntity;
constexpr int kDiamondId = 264;
constexpr int kDiamondCount = 7;
fs::path makeTempSavesDir(const std::string& name) {
 static std::atomic<int> counter{0};
 const fs::path root =
     fs::temp_directory_path() / "minecraft_native_save_recovery" / (name + "_" + std::to_string(++counter));
 std::error_code ec;
 fs::remove_all(root, ec);
 fs::create_directories(root, ec);
 return root;
}
// A player carrying one identifiable stack, far from origin so the position safeguard sees a
// real coordinate rather than the default-spawn sentinel.
void giveStockedInventory(ServerPlayerEntity& player) {
 player.inventory.main[0] = ItemStack(kDiamondId, kDiamondCount);
 player.setPosition(1200.5, 70.0, -840.5);
}
[[nodiscard]] int diamondsIn(const NbtCompound& playerNbt) {
 if(!playerNbt.contains("Inventory")) {
  return 0;
 }
 // Named local: getList returns by value, and until C++23 a range-for does not extend the
 // lifetime of a temporary the range expression only borrows from.
 const net::minecraft::NbtList inventory = playerNbt.getList("Inventory");
 for(const net::minecraft::Nbt& entryTag : inventory.entries()) {
  if(!entryTag.isCompound()) {
   continue;
  }
  const NbtCompound entry(entryTag);
  if(entry.getShort("id") == kDiamondId) {
   return entry.getByte("Count");
  }
 }
 return 0;
}
// Truncate a file to a prefix of its bytes — what a kill mid-write leaves behind. Half a gzip
// stream fails to inflate, so every reader of it takes the failure path.
void truncateToHalf(const fs::path& file) {
 std::string bytes;
 {
  std::ifstream input(file, std::ios::binary);
  bytes.assign(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
 }
 ASSERT_GT(bytes.size(), 8u);
 std::ofstream output(file, std::ios::binary | std::ios::trunc);
 output.write(bytes.data(), static_cast<std::streamsize>(bytes.size() / 2));
}
} // namespace
namespace net::minecraft::test {
// The regression behind "force end task corrupts my position and inventory". Autosave calls
// save() with the player carried inside the properties and no separate player list. That call
// used to resolve to an overload that wrote no Player compound at all, so every autosave
// erased the player the last graceful save had written -- and the next kill reloaded the world
// at spawn with an empty inventory.
TEST(PlayerSaveRecovery, AutosaveKeepsThePlayerInLevelDat) {
 const fs::path saves = makeTempSavesDir("autosave_keeps_player");
 ServerEventFixture fixture;
 server::network::ServerPlayerInteractionManager manager(&fixture.world);
 auto stocked = std::make_unique<ServerPlayerEntity>(&fixture.server, &fixture.world, "Eddie", &manager);
 giveStockedInventory(*stocked);
 AlphaWorldStorage storage(saves, "world", true);
 WorldProperties properties(12345, "world");
 NbtCompound playerNbt;
 stocked->writeNbt(playerNbt);
 properties.setPlayerNbt(playerNbt);
 storage.save(properties);
 const std::optional<WorldProperties> reloaded = storage.loadProperties();
 ASSERT_TRUE(reloaded.has_value());
 const NbtCompound* saved = reloaded->getPlayerNbt();
 ASSERT_NE(saved, nullptr);
 EXPECT_EQ(diamondsIn(*saved), kDiamondCount);
}
// A save with no player attached must not be the way the player disappears either: reload,
// save again, reload. The Player compound survives the round trip it is carried through.
TEST(PlayerSaveRecovery, ReloadedPlayerSurvivesTheNextSave) {
 const fs::path saves = makeTempSavesDir("player_round_trip");
 ServerEventFixture fixture;
 server::network::ServerPlayerInteractionManager manager(&fixture.world);
 auto stocked = std::make_unique<ServerPlayerEntity>(&fixture.server, &fixture.world, "Eddie", &manager);
 giveStockedInventory(*stocked);
 AlphaWorldStorage storage(saves, "world", true);
 WorldProperties properties(12345, "world");
 NbtCompound playerNbt;
 stocked->writeNbt(playerNbt);
 properties.setPlayerNbt(playerNbt);
 storage.save(properties);
 std::optional<WorldProperties> reloaded = storage.loadProperties();
 ASSERT_TRUE(reloaded.has_value());
 storage.save(*reloaded);
 reloaded = storage.loadProperties();
 ASSERT_TRUE(reloaded.has_value());
 ASSERT_NE(reloaded->getPlayerNbt(), nullptr);
 EXPECT_EQ(diamondsIn(*reloaded->getPlayerNbt()), kDiamondCount);
}
// World folds the live player into the properties on the one path every save shares. A player
// compound that has NOT been consumed by addPlayer() yet is the newer state -- the live entity
// has not read it -- so it wins over the blank pre-load player.
TEST(PlayerSaveRecovery, PendingLoadPayloadOutranksTheNotYetLoadedPlayer) {
 ServerEventFixture fixture;
 server::network::ServerPlayerInteractionManager manager(&fixture.world);
 auto stocked = std::make_unique<ServerPlayerEntity>(&fixture.server, &fixture.world, "Eddie", &manager);
 giveStockedInventory(*stocked);
 NbtCompound pending;
 stocked->writeNbt(pending);
 // The blank entity a fresh session creates before World::addPlayer() applies the save.
 auto blank = std::make_unique<ServerPlayerEntity>(&fixture.server, &fixture.world, "Eddie", &manager);
 blank->setPosition(0.0, 0.0, 0.0);
 fixture.world.players.push_back(blank.get());
 fixture.world.getProperties().setPlayerNbt(pending);
 const WorldProperties folded = fixture.world.propertiesWithPlayer();
 ASSERT_NE(folded.getPlayerNbt(), nullptr);
 EXPECT_EQ(diamondsIn(*folded.getPlayerNbt()), kDiamondCount);
 // Once addPlayer() has consumed the payload, the live player is the only source.
 fixture.world.getProperties().clearPlayerNbt();
 blank->inventory.main[0] = ItemStack(kDiamondId, kDiamondCount);
 const WorldProperties fromLive = fixture.world.propertiesWithPlayer();
 ASSERT_NE(fromLive.getPlayerNbt(), nullptr);
 EXPECT_EQ(diamondsIn(*fromLive.getPlayerNbt()), kDiamondCount);
 fixture.world.players.clear();
}
// Interrupted mid-write: level.dat is half a gzip stream, level.dat_old holds the last good
// copy. loadProperties() must reach past the damaged file to it.
TEST(PlayerSaveRecovery, TruncatedLevelDatLoadsFromTheBackup) {
 const fs::path saves = makeTempSavesDir("level_dat_truncated");
 ServerEventFixture fixture;
 server::network::ServerPlayerInteractionManager manager(&fixture.world);
 auto stocked = std::make_unique<ServerPlayerEntity>(&fixture.server, &fixture.world, "Eddie", &manager);
 giveStockedInventory(*stocked);
 AlphaWorldStorage storage(saves, "world", true);
 WorldProperties properties(12345, "world");
 NbtCompound playerNbt;
 stocked->writeNbt(playerNbt);
 properties.setPlayerNbt(playerNbt);
 // Twice, so the good copy is rotated into level.dat_old before level.dat is damaged.
 storage.save(properties);
 storage.save(properties);
 ASSERT_TRUE(fs::exists(saves / "world" / "level.dat_old"));
 ASSERT_NO_FATAL_FAILURE(truncateToHalf(saves / "world" / "level.dat"));
 const std::optional<WorldProperties> reloaded = storage.loadProperties();
 ASSERT_TRUE(reloaded.has_value());
 ASSERT_NE(reloaded->getPlayerNbt(), nullptr);
 EXPECT_EQ(diamondsIn(*reloaded->getPlayerNbt()), kDiamondCount);
}
// The multiplayer/dedicated side: players/<name>.dat had no backup fallback at all, so a
// truncated live file read as "no such player" and the next save wrote the empty result over
// the good backup.
TEST(PlayerSaveRecovery, TruncatedPlayerDatLoadsFromTheBackup) {
 const fs::path saves = makeTempSavesDir("player_dat_truncated");
 ServerEventFixture fixture;
 server::network::ServerPlayerInteractionManager manager(&fixture.world);
 auto stocked = std::make_unique<ServerPlayerEntity>(&fixture.server, &fixture.world, "Eddie", &manager);
 giveStockedInventory(*stocked);
 AlphaWorldStorage storage(saves, "world", true);
 storage.savePlayerData(*stocked);
 storage.savePlayerData(*stocked);
 const fs::path playerDat = saves / "world" / "players" / "Eddie.dat";
 ASSERT_TRUE(fs::exists(saves / "world" / "players" / "Eddie.dat_old"));
 ASSERT_NO_FATAL_FAILURE(truncateToHalf(playerDat));
 auto loaded = std::make_unique<ServerPlayerEntity>(&fixture.server, &fixture.world, "Eddie", &manager);
 storage.loadPlayerData(*loaded);
 EXPECT_EQ(loaded->inventory.main[0].itemId, kDiamondId);
 EXPECT_EQ(loaded->inventory.main[0].count, kDiamondCount);
}
// The swap window itself: the live file is gone entirely, only the backup survives.
TEST(PlayerSaveRecovery, MissingPlayerDatLoadsFromTheBackup) {
 const fs::path saves = makeTempSavesDir("player_dat_missing");
 ServerEventFixture fixture;
 server::network::ServerPlayerInteractionManager manager(&fixture.world);
 auto stocked = std::make_unique<ServerPlayerEntity>(&fixture.server, &fixture.world, "Eddie", &manager);
 giveStockedInventory(*stocked);
 AlphaWorldStorage storage(saves, "world", true);
 storage.savePlayerData(*stocked);
 storage.savePlayerData(*stocked);
 std::error_code ec;
 fs::remove(saves / "world" / "players" / "Eddie.dat", ec);
 auto loaded = std::make_unique<ServerPlayerEntity>(&fixture.server, &fixture.world, "Eddie", &manager);
 storage.loadPlayerData(*loaded);
 EXPECT_EQ(loaded->inventory.main[0].itemId, kDiamondId);
 EXPECT_EQ(loaded->inventory.main[0].count, kDiamondCount);
}
} // namespace net::minecraft::test
