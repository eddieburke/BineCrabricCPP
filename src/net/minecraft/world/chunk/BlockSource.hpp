#pragma once

#include "net/minecraft/block/Block.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace net::minecraft {

class BlockSource {
public:
    static void fill(std::vector<std::uint8_t>& blocks)
    {
        initializeBlocks();
        for (std::size_t i = 0; i < blocks.size(); ++i) {
            const std::uint8_t blockId = blocks[i];
            blocks[i] = Block::BLOCKS[static_cast<std::size_t>(blockId)] == nullptr ? 0 : blockId;
        }
    }

    static void fill(std::array<std::uint8_t, 256>& blocks)
    {
        initializeBlocks();
        for (std::size_t i = 0; i < blocks.size(); ++i) {
            const std::uint8_t blockId = static_cast<std::uint8_t>(i);
            blocks[i] = Block::BLOCKS[static_cast<std::size_t>(blockId)] == nullptr ? 0 : blockId;
        }
    }
};

} // namespace net::minecraft
