#pragma once
#include "net/minecraft/entity/EntityClientRendererDecl.hpp"
#include "net/minecraft/entity/passive/AnimalEntity.hpp"
#include "net/minecraft/nbt/NbtCompound.hpp"
namespace net::minecraft::entity::player {
class PlayerEntity;
}
namespace net::minecraft::entity::passive {
class SheepEntity : public AnimalEntity {
public:
  static constexpr int kEntityId = 91;
  static constexpr const char* kEntityName = "Sheep";
  struct ClientRenderer {
    static std::unique_ptr<::net::minecraft::client::render::entity::EntityRenderer> create();
  };
  explicit SheepEntity(World* world = nullptr);
  static constexpr float COLORS[16][3] = {
      {1.0f, 1.0f, 1.0f},
      {0.95f, 0.7f, 0.2f},
      {0.9f, 0.5f, 0.85f},
      {0.6f, 0.7f, 0.95f},
      {0.9f, 0.9f, 0.2f},
      {0.5f, 0.8f, 0.1f},
      {0.95f, 0.7f, 0.8f},
      {0.3f, 0.3f, 0.3f},
      {0.6f, 0.6f, 0.6f},
      {0.3f, 0.6f, 0.7f},
      {0.7f, 0.4f, 0.9f},
      {0.2f, 0.4f, 0.8f},
      {0.5f, 0.4f, 0.3f},
      {0.4f, 0.5f, 0.2f},
      {0.8f, 0.3f, 0.3f},
      {0.1f, 0.1f, 0.1f},
  };
  void initDataTracker() override {
    LivingEntity::initDataTracker();
    dataTracker.startTracking(16, static_cast<std::int8_t>(0));
  }
  void writeNbt(NbtCompound& nbt) const override;
  void readNbt(const NbtCompound& nbt) override;
  void dropItems() override;
  bool interact(player::PlayerEntity* player) override;
  void initializeSpawnState(JavaRandom& random) override;
  [[nodiscard]] int getColor() const {
    return dataTracker.getByte(16) & 0xF;
  }
  void setColor(int color) {
    const std::int8_t flags = dataTracker.getByte(16);
    dataTracker.set(16, static_cast<std::int8_t>((flags & 0xF0) | (color & 0xF)));
  }
  [[nodiscard]] bool isSheared() const {
    return (dataTracker.getByte(16) & 0x10) != 0;
  }
  void setSheared(bool sheared) {
    std::int8_t flags = dataTracker.getByte(16);
    if(sheared) {
      dataTracker.set(16, static_cast<std::int8_t>(flags | 0x10));
    } else {
      dataTracker.set(16, static_cast<std::int8_t>(flags & 0xFFFFFFEF));
    }
  }
  [[nodiscard]] std::string getRandomSound() override {
    return "mob.sheep";
  }
  [[nodiscard]] std::string getHurtSound() const override {
    return "mob.sheep";
  }
  [[nodiscard]] std::string getDeathSound() const override {
    return "mob.sheep";
  }
  static int generateDefaultColor(JavaRandom& random) {
    const int roll = random.nextInt(100);
    if(roll < 5) {
      return 15;
    }
    if(roll < 10) {
      return 7;
    }
    if(roll < 15) {
      return 8;
    }
    if(roll < 18) {
      return 12;
    }
    if(random.nextInt(500) == 0) {
      return 6;
    }
    return 0;
  }
};
} // namespace net::minecraft::entity::passive
