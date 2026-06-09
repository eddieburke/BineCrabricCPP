#pragma once

namespace net::minecraft::client::render::chunk {

// Per-chunk model transform applied while compiling a display list:
// translate to the (toroidally wrapped) render origin, then a tiny uniform
// scale about the chunk center to close seams between adjacent chunks.
struct ChunkDrawTransform {
    int renderX = 0;
    int renderY = 0;
    int renderZ = 0;
    int sizeX = 0;
    int sizeY = 0;
    int sizeZ = 0;

    static constexpr float kScale = 1.000001f;

    [[nodiscard]] static ChunkDrawTransform fromChunkPosition(
        int renderXIn, int renderYIn, int renderZIn, int sizeXIn, int sizeYIn, int sizeZIn) noexcept
    {
        return ChunkDrawTransform {renderXIn, renderYIn, renderZIn, sizeXIn, sizeYIn, sizeZIn};
    }

    void applyGl() const noexcept;
};

} // namespace net::minecraft::client::render::chunk
