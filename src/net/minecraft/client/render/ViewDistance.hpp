#pragma once

namespace net::minecraft::client::option {
class GameOptions;
}

namespace net::minecraft::client::render {

class ViewDistance {
public:
    enum class Preset {
        Far = 0,
        Normal = 1,
        Short = 2,
        Tiny = 3,
    };

    static constexpr float kNearClipBlocks = 0.05f;

    ViewDistance() = default;

    [[nodiscard]] static ViewDistance fromOptions(const option::GameOptions& options) noexcept;
    [[nodiscard]] static int baseDistanceBlocksForSetting(int setting) noexcept;

    [[nodiscard]] Preset preset() const noexcept { return preset_; }
    [[nodiscard]] int setting() const noexcept { return setting_; }
    [[nodiscard]] int baseDistanceBlocks() const noexcept { return baseDistanceBlocks_; }
    [[nodiscard]] float renderScale() const noexcept { return renderScale_; }
    [[nodiscard]] float renderDistanceBlocks() const noexcept { return renderDistanceBlocks_; }
    [[nodiscard]] float fogDistanceBlocks() const noexcept;
    [[nodiscard]] float projectionNearClipBlocks() const noexcept { return kNearClipBlocks; }
    [[nodiscard]] float projectionFarClipBlocks() const noexcept;
    [[nodiscard]] int chunkGridDiameterBlocks() const noexcept { return chunkGridDiameterBlocks_; }
    [[nodiscard]] int chunkGridRadiusChunks() const noexcept;
    [[nodiscard]] int chunkPreloadMarginChunks() const noexcept { return chunkPreloadMarginChunks_; }
    // Full client residency disc: renderer grid radius plus preload margin.
    [[nodiscard]] int worldChunkResidentRadiusChunks() const noexcept;
    [[nodiscard]] int worldChunkPreloadRadiusChunks() const noexcept { return worldChunkResidentRadiusChunks(); }
    [[nodiscard]] float fogColorBlendWeight() const noexcept;
    [[nodiscard]] float skyFogEndBlocks() const noexcept;
    [[nodiscard]] float worldFogStartBlocks(float factor) const noexcept;
    [[nodiscard]] float worldFogEndBlocks(float factor) const noexcept;
    [[nodiscard]] bool shouldRenderSky() const noexcept;

private:
    ViewDistance(Preset preset, int setting, int baseDistanceBlocks, float renderScale, float renderDistanceBlocks,
        int chunkGridDiameterBlocks,
        int chunkPreloadMarginChunks) noexcept;

    Preset preset_ = Preset::Far;
    int setting_ = 0;
    int baseDistanceBlocks_ = 256;
    float renderScale_ = 1.0f;
    float renderDistanceBlocks_ = 256.0f;
    int chunkGridDiameterBlocks_ = 400;
    int chunkPreloadMarginChunks_ = 2;
};

} // namespace net::minecraft::client::render
