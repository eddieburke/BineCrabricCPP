#pragma once
namespace net::minecraft {
class World;
}
namespace net::minecraft::mod::runtime {
class ModWorldDrawContext {
public:
  static void begin(net::minecraft::World* world, float tickDelta) noexcept;
  static void end() noexcept;
  [[nodiscard]] static net::minecraft::World* world() noexcept;
  [[nodiscard]] static float tickDelta() noexcept;
  [[nodiscard]] static bool active() noexcept;
};
class ScopedModWorldDrawContext {
public:
  ScopedModWorldDrawContext(net::minecraft::World* world, float tickDelta) noexcept;
  ~ScopedModWorldDrawContext();
  ScopedModWorldDrawContext(const ScopedModWorldDrawContext&) = delete;
  ScopedModWorldDrawContext& operator=(const ScopedModWorldDrawContext&) = delete;

private:
  bool entered_ = false;
};
} // namespace net::minecraft::mod::runtime
