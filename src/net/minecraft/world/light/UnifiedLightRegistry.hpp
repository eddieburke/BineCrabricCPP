#pragma once
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <shared_mutex>
#include <unordered_map>
#include <unordered_set>
#include <vector>
namespace net::minecraft {
class Chunk;
}
namespace net::minecraft::world::light {
enum class LightDomain : std::uint8_t {
  Block,
  Sun,
  Native
};
enum class LightShape : std::uint8_t {
  Point,
  Directional
};
struct LightKey {
  LightDomain domain = LightDomain::Native;
  int x = 0;
  int y = 0;
  int z = 0;
  std::uint64_t id = 0;
  [[nodiscard]] bool operator==(const LightKey&) const noexcept = default;
};
struct LightKeyHash {
  [[nodiscard]] std::size_t operator()(const LightKey& key) const noexcept;
};
struct PhysicalLight {
  LightKey key{};
  LightShape shape = LightShape::Point;
  double x = 0.0;
  double y = 0.0;
  double z = 0.0;
  float directionX = 0.0f;
  float directionY = 1.0f;
  float directionZ = 0.0f;
  float red = 1.0f;
  float green = 1.0f;
  float blue = 1.0f;
  float radius = 12.0f;
  float intensity = 1.0f;
  [[nodiscard]] bool operator==(const PhysicalLight&) const noexcept = default;
};
class UnifiedLightRegistry {
public:
  class ReadView {
  public:
    ReadView(ReadView&&) noexcept = default;
    ReadView& operator=(ReadView&&) noexcept = default;
    [[nodiscard]] const std::vector<PhysicalLight>& sources() const noexcept { return *sources_; }
    [[nodiscard]] const PhysicalLight* find(const LightKey& key) const noexcept;

  private:
    friend class UnifiedLightRegistry;
    ReadView(const UnifiedLightRegistry& registry);
    std::shared_lock<std::shared_mutex> lock_;
    const std::vector<PhysicalLight>* sources_;
  };
  static constexpr std::size_t kBlockProfileCount = 256;
  [[nodiscard]] static LightKey blockKey(int x, int y, int z) noexcept;
  [[nodiscard]] static LightKey sunKey() noexcept;
  static void setBlockEmission(int blockId, int emission) noexcept;
  [[nodiscard]] static int blockEmission(int blockId) noexcept;
  void syncBlockSource(int blockId, int x, int y, int z);
  void syncChunkSources(const Chunk& chunk);
  void eraseChunkSources(int chunkX, int chunkZ);
  void upsert(const LightKey& key, PhysicalLight source);
  bool erase(const LightKey& key);
  void clear();
  [[nodiscard]] ReadView read() const;
  [[nodiscard]] std::uint64_t revision() const noexcept { return revision_.load(std::memory_order_relaxed); }

private:
  inline static std::array<std::atomic<std::uint8_t>, kBlockProfileCount> blockEmission_{};
  mutable std::shared_mutex mutex_{};
  std::unordered_map<LightKey, std::size_t, LightKeyHash> indices_{};
  std::unordered_map<std::uint64_t, std::unordered_set<LightKey, LightKeyHash>> blockSourcesByChunk_{};
  std::vector<PhysicalLight> sources_{};
  std::atomic<std::uint64_t> revision_{0};
  [[nodiscard]] static std::uint64_t chunkKey(int chunkX, int chunkZ) noexcept;
  [[nodiscard]] static PhysicalLight makeBlockSource(const LightKey& key, int emission) noexcept;
  bool upsertLocked(const LightKey& key, PhysicalLight source);
  bool eraseLocked(const LightKey& key);
};
}
