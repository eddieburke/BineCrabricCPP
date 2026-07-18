#pragma once
#include <memory>
namespace net::minecraft::client::render::terrain {
class TerrainRenderer {
public:
  static TerrainRenderer& instance();
  TerrainRenderer(const TerrainRenderer&) = delete;
  TerrainRenderer& operator=(const TerrainRenderer&) = delete;
  void render(double cameraX, double cameraY, double cameraZ);
  void reset();

private:
  TerrainRenderer();
  ~TerrainRenderer();
  struct Impl;
  std::unique_ptr<Impl> impl_;
};
}
