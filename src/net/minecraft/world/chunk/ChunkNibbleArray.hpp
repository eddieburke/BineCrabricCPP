#pragma once

#include "net/minecraft/util/math/Types.hpp"

#include <algorithm>
#include <cstdint>
#include <vector>

namespace net::minecraft {

class ChunkNibbleArray {
public:
    explicit ChunkNibbleArray(int size)
        : bytes(static_cast<std::size_t>(size >> 1), 0)
    {
    }

    explicit ChunkNibbleArray(std::vector<std::uint8_t> data)
        : bytes(std::move(data))
    {
    }

    [[nodiscard]] int get(int x, int y, int z) const
    {
        const int index = (x << 11) | (z << 7) | y;
        const int byteIndex = index >> 1;
        if (byteIndex < 0 || static_cast<std::size_t>(byteIndex) >= bytes.size()) {
            return 0;
        }
        const int nibble = index & 1;
        if (nibble == 0) {
            return static_cast<int>(bytes[static_cast<std::size_t>(byteIndex)] & 0x0F);
        }
        return static_cast<int>((bytes[static_cast<std::size_t>(byteIndex)] >> 4) & 0x0F);
    }

    void set(int x, int y, int z, int value)
    {
        const int index = (x << 11) | (z << 7) | y;
        const int byteIndex = index >> 1;
        if (byteIndex < 0 || static_cast<std::size_t>(byteIndex) >= bytes.size()) {
            return;
        }
        const int nibble = index & 1;
        value &= 0x0F;
        std::uint8_t& byte = bytes[static_cast<std::size_t>(byteIndex)];
        if (nibble == 0) {
            byte = static_cast<std::uint8_t>((byte & 0xF0) | value);
        } else {
            byte = static_cast<std::uint8_t>((byte & 0x0F) | (static_cast<std::uint8_t>(value << 4)));
        }
    }

    void ensureSizeForBlockCount(std::size_t blockCount)
    {
        const std::size_t expectedBytes = blockCount / 2;
        if (bytes.size() == expectedBytes) {
            return;
        }
        std::vector<std::uint8_t> normalized(expectedBytes, 0);
        if (!bytes.empty()) {
            std::copy_n(bytes.begin(), std::min(bytes.size(), expectedBytes), normalized.begin());
        }
        bytes = std::move(normalized);
    }

    [[nodiscard]] bool isArrayInitialized() const noexcept
    {
        return !bytes.empty();
    }

    [[nodiscard]] bool hasExpectedSizeForBlockCount(std::size_t blockCount) const noexcept
    {
        return bytes.size() == blockCount / 2;
    }

    [[nodiscard]] bool isAllZero() const noexcept
    {
        for (std::uint8_t value : bytes) {
            if (value != 0) {
                return false;
            }
        }
        return true;
    }

    std::vector<std::uint8_t> bytes;
};

} // namespace net::minecraft
