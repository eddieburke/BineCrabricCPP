#pragma once
#include <algorithm>
#include <vector>

#include "net/minecraft/world/chunk/Chunk.hpp"

namespace net::minecraft {
class EmptyChunk : public Chunk {
   public:
    EmptyChunk(World* world = nullptr, int x = 0, int z = 0) : Chunk(world, x, z) {
        empty = true;
    }

    EmptyChunk(World* world, const std::vector<std::uint8_t>& blocks, int x, int z) : Chunk(world, blocks, x, z) {
        empty = true;
    }

    [[nodiscard]] int getHeight(int, int) const {
        return 0;
    }

    void populateHeightMapOnly() {
    }

    void populateHeightMap() {
    }

    void populateBlockLight() {
    }

    [[nodiscard]] int getBlockId(int, int, int) const {
        return 0;
    }

    bool setBlock(int, int, int, int, int) {
        return true;
    }

    bool setBlock(int, int, int, int) {
        return true;
    }

    [[nodiscard]] int getBlockMeta(int, int, int) const {
        return 0;
    }

    void setBlockMeta(int, int, int, int) {
    }

    [[nodiscard]] int getLight(LightType, int, int, int) const {
        return 0;
    }

    void setLight(LightType, int, int, int, int) {
    }

    [[nodiscard]] int getLight(int, int, int, int) const {
        return 0;
    }

    [[nodiscard]] bool isAboveMaxHeight(int, int, int) const {
        return false;
    }

    [[nodiscard]] bool shouldSave(bool) const {
        return false;
    }

    int loadFromPacket(
        const std::vector<std::uint8_t>&, int minX, int minY, int minZ, int maxX, int maxY, int maxZ, int offset) {
        const int volume = (maxX - minX) * (maxY - minY) * (maxZ - minZ);
        return offset + volume + volume / 2 * 3;
    }

    [[nodiscard]] int toPacket(std::vector<std::uint8_t>& bytes,
                               int minX,
                               int minY,
                               int minZ,
                               int maxX,
                               int maxY,
                               int maxZ,
                               int offset) const {
        const int volume = (maxX - minX) * (maxY - minY) * (maxZ - minZ);
        const int length = volume + volume / 2 * 3;
        if (bytes.size() < static_cast<std::size_t>(offset + length)) {
            bytes.resize(static_cast<std::size_t>(offset + length));
        }
        std::fill(bytes.begin() + offset, bytes.begin() + offset + length, static_cast<std::uint8_t>(0));
        return offset + length;
    }

    [[nodiscard]] bool isEmpty() const {
        return true;
    }
};
}  // namespace net::minecraft
