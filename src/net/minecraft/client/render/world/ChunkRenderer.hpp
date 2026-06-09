#pragma once

#include <vector>

namespace net::minecraft::client::render::world {

// Mirrors Java ChunkRenderer: buffers GL display-list IDs for a chunk region that shares
// the same camera-offset translation, then issues one glPushMatrix/glTranslatef/
// glCallLists/glPopMatrix for the entire group instead of one per chunk.
class ChunkRenderer {
public:
    void init(int x, int y, int z, double offsetX, double offsetY, double offsetZ) noexcept;

    [[nodiscard]] bool isAt(int x, int y, int z) const noexcept;

    void addGlList(int glList);

    void render();

    void clear() noexcept;

private:
    int x_ = 0;
    int y_ = 0;
    int z_ = 0;
    float offsetX_ = 0.0f;
    float offsetY_ = 0.0f;
    float offsetZ_ = 0.0f;
    bool initialized_ = false;
    std::vector<int> listBuffer_;
};

} // namespace net::minecraft::client::render::world
