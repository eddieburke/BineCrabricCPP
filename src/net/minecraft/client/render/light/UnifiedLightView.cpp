#include "net/minecraft/client/render/light/UnifiedLightView.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>
#include "net/minecraft/client/gl/GLCore.hpp"
#include "net/minecraft/client/gl/GlState.hpp"
#include "net/minecraft/client/gl/ShaderProgram.hpp"
#include "net/minecraft/client/render/FrameRenderCamera.hpp"
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/world/light/UnifiedLightRegistry.hpp"
namespace net::minecraft::client::render::light {
namespace {
namespace gl = net::minecraft::client::gl;
constexpr int kClusterSlices = 24;
void setupTexture(unsigned int texture, int width, int height) {
  gl::bindTexture(gl::cap::Texture2D, static_cast<int>(texture));
  gl::texImage2D(gl::cap::Texture2D, 0, gl::pixel::Rgba32f, width, height, 0,
                 gl::pixel::Rgba, gl::pixel::Float, nullptr);
  gl::texParameteri(gl::cap::Texture2D, gl::tex::MinFilter, gl::filter::Nearest);
  gl::texParameteri(gl::cap::Texture2D, gl::tex::MagFilter, gl::filter::Nearest);
  gl::texParameteri(gl::cap::Texture2D, gl::tex::WrapS, gl::wrap::ClampToEdge);
  gl::texParameteri(gl::cap::Texture2D, gl::tex::WrapT, gl::wrap::ClampToEdge);
}
void setupIndexTexture(unsigned int texture, int width, int height) {
  gl::bindTexture(gl::cap::Texture2D, static_cast<int>(texture));
  gl::texImage2D(gl::cap::Texture2D, 0, gl::pixel::Rgba32ui, width, height, 0,
                 gl::pixel::RgbaInteger, gl::pixel::UnsignedInt, nullptr);
  gl::texParameteri(gl::cap::Texture2D, gl::tex::MinFilter, gl::filter::Nearest);
  gl::texParameteri(gl::cap::Texture2D, gl::tex::MagFilter, gl::filter::Nearest);
  gl::texParameteri(gl::cap::Texture2D, gl::tex::WrapS, gl::wrap::ClampToEdge);
  gl::texParameteri(gl::cap::Texture2D, gl::tex::WrapT, gl::wrap::ClampToEdge);
}
[[nodiscard]] bool floatTexturesAvailable() {
  if(gl::GLCore::activeTexture == nullptr) {
    return false;
  }
  const auto* version = ::glGetString(0x1F02);
  if(version == nullptr) {
    return false;
  }
  int units = 0;
  gl::getIntegerv(gl::tex::MaxTextureImageUnits, &units);
  const char first = static_cast<char>(version[0]);
  return units >= 4 && first >= '3' && first <= '9';
}
}
struct UnifiedLightView::State {
  ::net::minecraft::world::light::UnifiedLightRegistry* registry = nullptr;
  unsigned int positions = 0;
  unsigned int colors = 0;
  unsigned int tileGrid = 0;
  unsigned int tileIndices = 0;
  bool gpuReady = false;
  int lightCount = 0;
  int textureWidth = 1;
  int textureHeight = 1;
  int textureCapacity = 0;
  int gridWidth = 0;
  int gridHeight = 0;
  int indexWidth = 0;
  int indexHeight = 0;
  int indexCapacity = 0;
  double eyeX = 0.0;
  double eyeY = 0.0;
  double eyeZ = 0.0;
};
namespace {
template <typename StateT>
void ensureTextures(StateT& state, int count) {
  gl::GLCore::ensureLoaded();
  if(!floatTexturesAvailable()) {
    return;
  }
  int maxTextureSize = 0;
  gl::getIntegerv(0x0D33, &maxTextureSize);
  const int width = std::max(1, std::min(std::max(1, maxTextureSize), 2048));
  const int height = std::max(1, (count + width - 1) / width);
  if(state.gpuReady && count <= state.textureCapacity && state.textureWidth == width) {
    return;
  }
  int previousActive = gl::tex::Texture0;
  int binding = 0;
  gl::getIntegerv(gl::tex::ActiveTexture, &previousActive);
  gl::activeTexture(gl::tex::Texture0);
  gl::getIntegerv(gl::tex::Binding2D, &binding);
  if(state.positions != 0 || state.colors != 0 || state.tileGrid != 0 || state.tileIndices != 0) {
    const unsigned int oldTextures[4]{state.positions, state.colors, state.tileGrid, state.tileIndices};
    gl::deleteTextures(4, oldTextures);
    state.positions = 0;
    state.colors = 0;
    state.tileGrid = 0;
    state.tileIndices = 0;
    state.gridWidth = 0;
    state.gridHeight = 0;
    state.indexWidth = 0;
    state.indexHeight = 0;
    state.indexCapacity = 0;
  }
  unsigned int textures[4]{};
  while(::glGetError() != 0) {
  }
  gl::genTextures(4, textures);
  state.positions = textures[0];
  state.colors = textures[1];
  state.tileGrid = textures[2];
  state.tileIndices = textures[3];
  if(state.positions != 0 && state.colors != 0 && state.tileGrid != 0 && state.tileIndices != 0) {
    setupTexture(state.positions, width, height);
    setupTexture(state.colors, width, height);
  }
  state.gpuReady = state.positions != 0 && state.colors != 0 && state.tileGrid != 0 && state.tileIndices != 0 &&
                   ::glGetError() == 0;
  if(!state.gpuReady) {
    gl::deleteTextures(4, textures);
    state.positions = 0;
    state.colors = 0;
    state.tileGrid = 0;
    state.tileIndices = 0;
  } else {
    state.textureWidth = width;
    state.textureHeight = height;
    state.textureCapacity = width * height;
  }
  gl::bindTexture(gl::cap::Texture2D, binding);
  gl::activeTexture(previousActive);
}
}
UnifiedLightView::UnifiedLightView() : state_(std::make_unique<State>()) {}
UnifiedLightView::~UnifiedLightView() { destroy(); }
UnifiedLightView::UnifiedLightView(UnifiedLightView&&) noexcept = default;
UnifiedLightView& UnifiedLightView::operator=(UnifiedLightView&&) noexcept = default;
void UnifiedLightView::update(World* world, double eyeX, double eyeY, double eyeZ) {
  State& state = *state_;
  state.registry = world != nullptr ? &world->lightRegistry() : nullptr;
  state.eyeX = eyeX;
  state.eyeY = eyeY;
  state.eyeZ = eyeZ;
  if(state.registry == nullptr) {
    state.lightCount = 0;
  }
}
bool UnifiedLightView::bind(gl::ShaderProgram& shader,
                         int textureUnit,
                         const FrameRenderCamera& camera,
                         int viewportWidth,
                         int viewportHeight) {
  State& state = *state_;
  if(!shader.valid() || textureUnit < 0 || gl::GLCore::activeTexture == nullptr || state.registry == nullptr) {
    return false;
  }
  int units = 0;
  gl::getIntegerv(gl::tex::MaxTextureImageUnits, &units);
  if(textureUnit + 4 > units || viewportWidth <= 0 || viewportHeight <= 0) {
    return false;
  }
  auto registryView = state.registry->read();
  const auto& sources = registryView.sources();
  std::vector<std::size_t> active;
  active.reserve(sources.size());
  for(std::size_t sourceIndex = 0; sourceIndex < sources.size(); ++sourceIndex) {
    const ::net::minecraft::world::light::PhysicalLight& source = sources[sourceIndex];
    if(source.shape == ::net::minecraft::world::light::LightShape::Point && source.intensity > 0.0f) {
      active.push_back(sourceIndex);
    }
  }
  state.lightCount = static_cast<int>(active.size());
  ensureTextures(state, state.lightCount);
  if(!state.gpuReady) {
    return false;
  }
  std::vector<float> positions(static_cast<std::size_t>(state.textureCapacity) * 4);
  std::vector<float> colors(static_cast<std::size_t>(state.textureCapacity) * 4);
  for(std::size_t index = 0; index < active.size(); ++index) {
    const ::net::minecraft::world::light::PhysicalLight& source = sources[active[index]];
    const std::size_t base = index * 4;
    positions[base] = static_cast<float>(source.x - state.eyeX);
    positions[base + 1] = static_cast<float>(source.y - state.eyeY);
    positions[base + 2] = static_cast<float>(source.z - state.eyeZ);
    positions[base + 3] = source.radius;
    colors[base] = source.red;
    colors[base + 1] = source.green;
    colors[base + 2] = source.blue;
    colors[base + 3] = source.intensity;
  }
  int activeTexture = gl::tex::Texture0;
  int pointBinding = 0;
  gl::getIntegerv(gl::tex::ActiveTexture, &activeTexture);
  gl::activeTexture(gl::tex::Texture0);
  gl::getIntegerv(gl::tex::Binding2D, &pointBinding);
  gl::bindTexture(gl::cap::Texture2D, static_cast<int>(state.positions));
  gl::texSubImage2D(gl::cap::Texture2D, 0, 0, 0, state.textureWidth, state.textureHeight,
                    gl::pixel::Rgba, gl::pixel::Float, positions.data());
  gl::bindTexture(gl::cap::Texture2D, static_cast<int>(state.colors));
  gl::texSubImage2D(gl::cap::Texture2D, 0, 0, 0, state.textureWidth, state.textureHeight,
                    gl::pixel::Rgba, gl::pixel::Float, colors.data());
  gl::bindTexture(gl::cap::Texture2D, pointBinding);
  gl::activeTexture(activeTexture);
  const int tileColumns = (viewportWidth + kTileSize - 1) / kTileSize;
  const int tileRows = (viewportHeight + kTileSize - 1) / kTileSize;
  struct TileBounds {
    int lightIndex;
    int minX;
    int maxX;
    int minY;
    int maxY;
    int minZ;
    int maxZ;
  };
  const std::size_t tileCount = static_cast<std::size_t>(tileColumns) * static_cast<std::size_t>(tileRows);
  const std::size_t clusterCount = tileCount * kClusterSlices;
  std::vector<TileBounds> bounds;
  bounds.reserve(active.size());
  std::vector<std::size_t> counts(clusterCount);
  const float nearPlane = std::max(0.001f, camera.perspectiveNear);
  const float farPlane = camera.perspectiveFar > nearPlane ? camera.perspectiveFar : 1.0e30f;
  const float clusterScale = static_cast<float>(kClusterSlices) / std::log2(farPlane / nearPlane);
  const float clusterBias = -std::log2(nearPlane) * clusterScale;
  const auto depthSlice = [&](float depth) {
    return std::clamp(static_cast<int>(std::log2(std::max(depth, nearPlane)) * clusterScale + clusterBias),
                      0,
                      kClusterSlices - 1);
  };
  for(std::size_t index = 0; index < active.size(); ++index) {
    const ::net::minecraft::world::light::PhysicalLight& light = sources[active[index]];
    const float relativeX = static_cast<float>(light.x - state.eyeX);
    const float relativeY = static_cast<float>(light.y - state.eyeY);
    const float relativeZ = static_cast<float>(light.z - state.eyeZ);
    const float viewX = relativeX * camera.viewRightX + relativeY * camera.viewRightY + relativeZ * camera.viewRightZ;
    const float viewY = relativeX * camera.viewUpX + relativeY * camera.viewUpY + relativeZ * camera.viewUpZ;
    const float forward = relativeX * camera.viewForwardX + relativeY * camera.viewForwardY + relativeZ * camera.viewForwardZ;
    if(forward + light.radius <= nearPlane || forward - light.radius >= farPlane) {
      continue;
    }
    const int minSlice = depthSlice(std::max(nearPlane, forward - light.radius));
    const int maxSlice = depthSlice(std::min(farPlane, forward + light.radius));
    int minTileX = 0;
    int maxTileX = tileColumns - 1;
    int minTileY = 0;
    int maxTileY = tileRows - 1;
    if(forward > light.radius + nearPlane) {
      const float centerX = (viewX * camera.projectionX / forward * 0.5f + 0.5f) * viewportWidth;
      const float centerY = (viewY * camera.projectionY / forward * 0.5f + 0.5f) * viewportHeight;
      const float nearestDepth = std::max(nearPlane, forward - light.radius);
      const float radiusX = camera.projectionX * light.radius / nearestDepth * viewportWidth * 0.5f;
      const float radiusY = camera.projectionY * light.radius / nearestDepth * viewportHeight * 0.5f;
      minTileX = std::clamp(static_cast<int>(std::floor((centerX - radiusX) / kTileSize)), 0, tileColumns - 1);
      maxTileX = std::clamp(static_cast<int>(std::floor((centerX + radiusX) / kTileSize)), 0, tileColumns - 1);
      minTileY = std::clamp(static_cast<int>(std::floor((centerY - radiusY) / kTileSize)), 0, tileRows - 1);
      maxTileY = std::clamp(static_cast<int>(std::floor((centerY + radiusY) / kTileSize)), 0, tileRows - 1);
      if(centerX + radiusX < 0.0f || centerX - radiusX >= viewportWidth ||
         centerY + radiusY < 0.0f || centerY - radiusY >= viewportHeight) {
        continue;
      }
    }
    bounds.push_back({static_cast<int>(index), minTileX, maxTileX, minTileY, maxTileY, minSlice, maxSlice});
    for(int slice = minSlice; slice <= maxSlice; ++slice) {
      for(int tileY = minTileY; tileY <= maxTileY; ++tileY) {
        for(int tileX = minTileX; tileX <= maxTileX; ++tileX) {
          ++counts[static_cast<std::size_t>((slice * tileRows + tileY) * tileColumns + tileX)];
        }
      }
    }
  }
  std::vector<std::size_t> offsets(clusterCount + 1);
  for(std::size_t cluster = 0; cluster < clusterCount; ++cluster) {
    offsets[cluster + 1] = offsets[cluster] + counts[cluster];
  }
  const std::size_t intersectionCount = offsets.back();
  int maxTextureSize = 0;
  gl::getIntegerv(0x0D33, &maxTextureSize);
  const int indexWidth = std::max(1, std::min(std::max(1, maxTextureSize), 2048));
  const std::size_t indexTexelsWide = std::max<std::size_t>(1, (intersectionCount + 3) / 4);
  const std::size_t indexHeightWide = (indexTexelsWide + static_cast<std::size_t>(indexWidth) - 1) / static_cast<std::size_t>(indexWidth);
  const int clusterRows = tileRows * kClusterSlices;
  if(maxTextureSize <= 0 || tileColumns > maxTextureSize || clusterRows > maxTextureSize ||
     indexHeightWide > static_cast<std::size_t>(maxTextureSize)) {
    return false;
  }
  const int indexHeight = static_cast<int>(indexHeightWide);
  std::vector<std::uint32_t> grid(clusterCount * 4);
  std::vector<std::uint32_t> indices(
      std::max<std::size_t>(4, ((intersectionCount + 3) / 4) * 4), UINT32_MAX);
  std::vector<std::size_t> cursors(offsets.begin(), offsets.end() - 1);
  for(std::size_t cluster = 0; cluster < clusterCount; ++cluster) {
    grid[cluster * 4] = static_cast<std::uint32_t>(offsets[cluster]);
    grid[cluster * 4 + 1] = static_cast<std::uint32_t>(counts[cluster]);
  }
  for(const TileBounds& bound : bounds) {
    for(int slice = bound.minZ; slice <= bound.maxZ; ++slice) {
      for(int tileY = bound.minY; tileY <= bound.maxY; ++tileY) {
        for(int tileX = bound.minX; tileX <= bound.maxX; ++tileX) {
          const std::size_t cluster =
              static_cast<std::size_t>((slice * tileRows + tileY) * tileColumns + tileX);
          indices[cursors[cluster]++] = static_cast<std::uint32_t>(bound.lightIndex);
        }
      }
    }
  }
  indices.resize(static_cast<std::size_t>(indexWidth * indexHeight) * 4, UINT32_MAX);
  int previousActive = gl::tex::Texture0;
  int binding = 0;
  gl::getIntegerv(gl::tex::ActiveTexture, &previousActive);
  gl::activeTexture(gl::tex::Texture0);
  gl::getIntegerv(gl::tex::Binding2D, &binding);
  gl::bindTexture(gl::cap::Texture2D, static_cast<int>(state.tileGrid));
  if(state.gridWidth != tileColumns || state.gridHeight != clusterRows) {
    setupIndexTexture(state.tileGrid, tileColumns, clusterRows);
    state.gridWidth = tileColumns;
    state.gridHeight = clusterRows;
  }
  gl::texSubImage2D(gl::cap::Texture2D, 0, 0, 0, tileColumns, clusterRows,
                    gl::pixel::RgbaInteger, gl::pixel::UnsignedInt, grid.data());
  gl::bindTexture(gl::cap::Texture2D, static_cast<int>(state.tileIndices));
  if(state.indexWidth != indexWidth || state.indexCapacity < indexWidth * indexHeight) {
    setupIndexTexture(state.tileIndices, indexWidth, indexHeight);
    state.indexWidth = indexWidth;
    state.indexHeight = indexHeight;
    state.indexCapacity = indexWidth * indexHeight;
  }
  gl::texSubImage2D(gl::cap::Texture2D, 0, 0, 0, indexWidth, indexHeight,
                    gl::pixel::RgbaInteger, gl::pixel::UnsignedInt, indices.data());
  gl::bindTexture(gl::cap::Texture2D, binding);
  gl::activeTexture(gl::tex::Texture0 + textureUnit);
  gl::bindTexture(gl::cap::Texture2D, static_cast<int>(state.positions));
  gl::activeTexture(gl::tex::Texture0 + textureUnit + 1);
  gl::bindTexture(gl::cap::Texture2D, static_cast<int>(state.colors));
  gl::activeTexture(gl::tex::Texture0 + textureUnit + 2);
  gl::bindTexture(gl::cap::Texture2D, static_cast<int>(state.tileGrid));
  gl::activeTexture(gl::tex::Texture0 + textureUnit + 3);
  gl::bindTexture(gl::cap::Texture2D, static_cast<int>(state.tileIndices));
  gl::activeTexture(previousActive);
  shader.set1i("lightData0", textureUnit);
  shader.set1i("lightData1", textureUnit + 1);
  shader.set1i("lightTileGrid", textureUnit + 2);
  shader.set1i("lightTileIndices", textureUnit + 3);
  shader.set1i("lightCount", state.lightCount);
  shader.set2f("uLightDataSize", static_cast<float>(state.textureWidth), static_cast<float>(state.textureHeight));
  shader.set2f("uLightTileCount", static_cast<float>(tileColumns), static_cast<float>(tileRows));
  shader.set2f("uLightClusterParams", clusterScale, clusterBias);
  shader.set1i("uLightClusterSlices", kClusterSlices);
  shader.set2f("uLightIndexSize", static_cast<float>(indexWidth), static_cast<float>(indexHeight));
  shader.set1f("uLightTileSize", static_cast<float>(kTileSize));
  return true;
}
void UnifiedLightView::reset() {
  state_->registry = nullptr;
  state_->lightCount = 0;
}
void UnifiedLightView::destroy() {
  if(!state_) {
    return;
  }
  if(state_->positions != 0 || state_->colors != 0 || state_->tileGrid != 0 || state_->tileIndices != 0) {
    const unsigned int textures[4]{state_->positions, state_->colors, state_->tileGrid, state_->tileIndices};
    gl::deleteTextures(4, textures);
  }
  state_->positions = 0;
  state_->colors = 0;
  state_->tileGrid = 0;
  state_->tileIndices = 0;
  state_->gpuReady = false;
  state_->lightCount = 0;
}
bool UnifiedLightView::ready() const { return state_->gpuReady; }
int UnifiedLightView::count() const { return state_->lightCount; }
unsigned int UnifiedLightView::positionTexture() const { return state_->positions; }
unsigned int UnifiedLightView::colorTexture() const { return state_->colors; }
}
