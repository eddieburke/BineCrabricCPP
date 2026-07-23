#include "net/minecraft/world/dimension/Dimension.hpp"
#include <cmath>
#include "net/minecraft/util/math/MathHelper.hpp"
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/world/chunk/ChunkSource.hpp"
#include "net/minecraft/world/dimension/DimensionType.hpp"
namespace net::minecraft {
Dimension::Dimension(const DimensionType& type) : id(type.id), type_(&type) {
 isNether = type.isNether;
 evaporatesWater = type.evaporatesWater;
 hasCeiling = type.hasCeiling;
}
const DimensionType& Dimension::resolvedType() const {
 return *type_;
}
void Dimension::setWorld(World* worldIn) {
 world = worldIn;
 initBiomeSource();
 initBrightnessTable();
}
void Dimension::initBrightnessTable() {
 const float factor = type_->brightnessFactor;
 for(int i = 0; i <= 15; ++i) {
  const float value = 1.0f - static_cast<float>(i) / 15.0f;
  lightLevelToLuminance[static_cast<std::size_t>(i)] =
      (1.0f - value) / (value * 3.0f + 1.0f) * (1.0f - factor) + factor;
 }
}
float Dimension::luminanceForLightLevel(int lightLevel) {
 if(lightLevel < 0) {
  lightLevel = 0;
 } else if(lightLevel > 15) {
  lightLevel = 15;
 }
 constexpr float factor = 0.05f;
 const float value = 1.0f - static_cast<float>(lightLevel) / 15.0f;
 return (1.0f - value) / (value * 3.0f + 1.0f) * (1.0f - factor) + factor;
}
void Dimension::initBiomeSource() {
 if(world == nullptr) {
  return;
 }
 biomeSource = resolvedType().makeBiomeSource(world);
}
std::unique_ptr<ChunkSource> Dimension::createChunkGenerator() {
 if(world == nullptr) {
  return nullptr;
 }
 return resolvedType().makeGenerator(world, world->seed(), /*localBiomeSource=*/false);
}
std::unique_ptr<ChunkSource> Dimension::createChunkGeneratorFromSeed(std::uint64_t seed) {
 if(world == nullptr) {
  return nullptr;
 }
 return resolvedType().makeGenerator(world, seed, /*localBiomeSource=*/true);
}
double Dimension::movementFactor() const {
 return type_->movementFactor;
}
bool Dimension::hasWorldSpawn() const {
 return type_->hasWorldSpawn;
}
float Dimension::getCloudHeight() const {
 return type_->cloudHeight;
}
bool Dimension::hasGround() const {
 return type_->hasGround;
}
float Dimension::getTimeOfDay(long long time, float tickDelta) const {
 if(type_->fixedTime) {
  return type_->fixedTimeOfDay;
 }
 const int timeOfDay = static_cast<int>(time % 24000LL);
 float value = (static_cast<float>(timeOfDay) + tickDelta) / 24000.0f - 0.25f;
 if(value < 0.0f) {
  value += 1.0f;
 }
 if(value > 1.0f) {
  value -= 1.0f;
 }
 return value;
}
std::array<float, 4>* Dimension::getBackgroundColor(float timeOfDay, float) {
 if(!type_->hasBackgroundColor) {
  return nullptr;
 }
 constexpr float range = 0.4f;
 const float cosine = MathHelper::cos(timeOfDay * 3.14159265f * 2.0f);
 if(cosine >= -range && cosine <= range) {
  const float blend = (cosine / range) * 0.5f + 0.5f;
  float alpha = 1.0f - (1.0f - MathHelper::sin(blend * 3.14159265f)) * 0.99f;
  alpha *= alpha;
  backgroundColor_[0] = blend * 0.3f + 0.7f;
  backgroundColor_[1] = blend * blend * 0.7f + 0.2f;
  backgroundColor_[2] = blend * blend * 0.0f + 0.2f;
  backgroundColor_[3] = alpha;
  return &backgroundColor_;
 }
 return nullptr;
}
Vec3d Dimension::getFogColor(float timeOfDay, float) const {
 return type_->fogColor(timeOfDay);
}
bool Dimension::isValidSpawnPoint(int x, int z) const {
 if(world == nullptr) {
  return false;
 }
 return type_->isValidSpawn(*world, x, z);
}
std::unique_ptr<Dimension> Dimension::fromId(int dimensionId) {
 const DimensionType* type = dimension::dimensionById(dimensionId);
 if(type == nullptr) {
  return nullptr;
 }
 return std::make_unique<Dimension>(*type);
}
} // namespace net::minecraft
