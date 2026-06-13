#include "net/minecraft/client/render/ViewDistance.hpp"

#include "net/minecraft/client/option/GameOptions.hpp"

#include <algorithm>
#include <cmath>

namespace net::minecraft::client::render {
namespace {

constexpr int kChunkSectionSize = 16;
constexpr int kChunkGridCapBlocks = 400;
constexpr int kMinChunkGridDiameterBlocks = 64;
constexpr int kMaxChunkGridDiameterBlocks = 2000;
// Margin between the renderer's section radius and the world resident radius, so
// the leading edge always has resident neighbors to mesh against while moving.
constexpr int kDefaultChunkPreloadMarginChunks = 3;
constexpr float kSkyFogEndFactor = 0.8f;

[[nodiscard]] int normalizeSetting(int setting) noexcept
{
    return setting & 3;
}

[[nodiscard]] ViewDistance::Preset presetFromSetting(int setting) noexcept
{
    return static_cast<ViewDistance::Preset>(normalizeSetting(setting));
}

[[nodiscard]] int chunkGridDiameterForBaseDistance(int baseDistanceBlocks, float renderScale) noexcept
{
    // Match the old resolved-options behavior: cap the far preset to the
    // legacy 400-block torus before applying the OptiFine-style scale slider.
    const int baseDiameter = std::min(baseDistanceBlocks * 2, kChunkGridCapBlocks);
    const int scaledDiameter = static_cast<int>(static_cast<float>(baseDiameter) * renderScale);
    return std::clamp(scaledDiameter, kMinChunkGridDiameterBlocks, kMaxChunkGridDiameterBlocks);
}

[[nodiscard]] float normalizeRenderScale(float renderScale) noexcept
{
    if (!std::isfinite(renderScale)) {
        return 1.0f;
    }
    return std::clamp(renderScale, 1.0f, 5.0f);
}

[[nodiscard]] int chunkPreloadMargin(const option::GameOptions& options) noexcept
{
    return options.ofPreloadedChunks <= 0
        ? kDefaultChunkPreloadMarginChunks
        : kDefaultChunkPreloadMarginChunks + options.ofPreloadedChunks / 2;
}

} // namespace

ViewDistance::ViewDistance(Preset preset, int setting, int baseDistanceBlocks, float renderScale,
    float renderDistanceBlocks, int chunkGridDiameterBlocks, int chunkPreloadMarginChunks) noexcept
    : preset_(preset)
    , setting_(setting)
    , baseDistanceBlocks_(baseDistanceBlocks)
    , renderScale_(renderScale)
    , renderDistanceBlocks_(renderDistanceBlocks)
    , chunkGridDiameterBlocks_(chunkGridDiameterBlocks)
    , chunkPreloadMarginChunks_(chunkPreloadMarginChunks)
{
}

ViewDistance ViewDistance::fromOptions(const option::GameOptions& options) noexcept
{
    const int setting = normalizeSetting(options.viewDistance);
    const int baseDistance = baseDistanceBlocksForSetting(setting);
    const float renderScale = normalizeRenderScale(options.ofRenderScale);
    const float renderDistance = static_cast<float>(baseDistance) * renderScale;
    return ViewDistance {
        presetFromSetting(setting),
        setting,
        baseDistance,
        renderScale,
        renderDistance,
        chunkGridDiameterForBaseDistance(baseDistance, renderScale),
        chunkPreloadMargin(options)};
}

int ViewDistance::baseDistanceBlocksForSetting(int setting) noexcept
{
    return 256 >> normalizeSetting(setting);
}

float ViewDistance::fogDistanceBlocks() const noexcept
{
    return renderDistanceBlocks_;
}

float ViewDistance::projectionFarClipBlocks() const noexcept
{
    return static_cast<float>(renderDistanceBlocks_) * 2.0f;
}

int ViewDistance::chunkGridRadiusChunks() const noexcept
{
    const int chunkGridWidthChunks = chunkGridDiameterBlocks_ / kChunkSectionSize + 1;
    return chunkGridWidthChunks / 2;
}

int ViewDistance::worldChunkResidentRadiusChunks() const noexcept
{
    return chunkGridRadiusChunks() + chunkPreloadMarginChunks_;
}

float ViewDistance::fogColorBlendWeight() const noexcept
{
    const float distanceBlend = 1.0f / static_cast<float>(4 - setting_);
    return 1.0f - static_cast<float>(std::pow(static_cast<double>(distanceBlend), 0.25));
}

float ViewDistance::skyFogEndBlocks() const noexcept
{
    return fogDistanceBlocks() * kSkyFogEndFactor;
}

float ViewDistance::worldFogStartBlocks(float factor) const noexcept
{
    return fogDistanceBlocks() * factor;
}

float ViewDistance::worldFogEndBlocks(float factor) const noexcept
{
    return fogDistanceBlocks() * factor;
}

bool ViewDistance::shouldRenderSky() const noexcept
{
    return setting_ < 2;
}

} // namespace net::minecraft::client::render
