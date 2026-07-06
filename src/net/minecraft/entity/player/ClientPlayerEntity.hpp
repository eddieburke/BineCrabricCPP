#pragma once
#include "net/minecraft/client/option/GameOptions.hpp"
#include "net/minecraft/client/util/Session.hpp"
#include "net/minecraft/client/util/SmoothUtil.hpp"
#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/inventory/DoubleInventory.hpp"
#include <memory>
#include <string>
namespace net::minecraft::client {
class Minecraft;
}
namespace net::minecraft::entity::player {
class ClientPlayerEntity : public PlayerEntity {
public:
  ClientPlayerEntity(client::Minecraft* minecraft, World* world, const client::util::Session& session,
                     client::option::GameOptions& options, int dimensionIdIn);
  void move(double dx, double dy, double dz) override;
  void tickLiving() override;
  void tickMovement() override;
  void writeNbt(NbtCompound& nbt) const override;
  void readNbt(const NbtCompound& nbt) override;
  void closeHandledScreen() override;
  void openChestScreen(Inventory* inventoryIn) override;
  void openChestScreen(int xIn, int yIn, int zIn) override;
  void openFurnaceScreen(block::entity::FurnaceBlockEntity* furnaceIn) override;
  void openDispenserScreen(block::entity::DispenserBlockEntity* dispenserIn) override;
  void openCraftingScreen(int xIn, int yIn, int zIn) override;
  [[nodiscard]] bool isSneaking() const override;
  void respawn() override;
  void increaseStat(int stat, int amount) override;

private:
  [[nodiscard]] bool shouldSuffocate(int blockX, int blockY, int blockZ) const;
  bool pushOutOfBlock(double px, double py, double pz);
  client::Minecraft* minecraft_ = nullptr;
  client::util::SmoothUtil field163;
  client::util::SmoothUtil field164;
  client::util::SmoothUtil field165;
  std::unique_ptr<::net::minecraft::DoubleInventory> openChestDoubleInventory_;
};
} // namespace net::minecraft::entity::player
