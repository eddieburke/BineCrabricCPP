#pragma once

#include "net/minecraft/network/NetworkHandler.hpp"
#include "net/minecraft/network/Packet.hpp"
#include "net/minecraft/network/PacketIO.hpp"
#include "net/minecraft/util/math/Types.hpp"

#include <cstdint>
#include <cstddef>
#include <istream>
#include <ostream>
#include <vector>

namespace net::minecraft {

class BlockUpdateS2CPacket : public Packet {
public:
    int x = 0;
    int y = 0;
    int z = 0;
    int blockRawId = 0;
    int blockMetadata = 0;

    BlockUpdateS2CPacket() { worldPacket = true; }

    void read(std::istream& input) override
    {
        x = packetio::readI32BE(input);
        y = packetio::readU8(input);
        z = packetio::readI32BE(input);
        blockRawId = packetio::readU8(input);
        blockMetadata = packetio::readU8(input);
    }

    void write(std::ostream& output) const override
    {
        packetio::writeI32BE(output, x);
        packetio::writeU8(output, static_cast<std::uint8_t>(y));
        packetio::writeI32BE(output, z);
        packetio::writeU8(output, static_cast<std::uint8_t>(blockRawId));
        packetio::writeU8(output, static_cast<std::uint8_t>(blockMetadata));
    }

    void apply(NetworkHandler& networkHandler) const override { networkHandler.onBlockUpdate(*this); }
    [[nodiscard]] std::size_t size() const override { return 11; }
};

class PlayNoteSoundS2CPacket : public Packet {
public:
    int x = 0;
    int y = 0;
    int z = 0;
    int instrument = 0;
    int pitch = 0;

    void read(std::istream& input) override
    {
        x = packetio::readI32BE(input);
        y = packetio::readI16BE(input);
        z = packetio::readI32BE(input);
        instrument = packetio::readU8(input);
        pitch = packetio::readU8(input);
    }

    void write(std::ostream& output) const override
    {
        packetio::writeI32BE(output, x);
        packetio::writeI16BE(output, static_cast<std::int16_t>(y));
        packetio::writeI32BE(output, z);
        packetio::writeU8(output, static_cast<std::uint8_t>(instrument));
        packetio::writeU8(output, static_cast<std::uint8_t>(pitch));
    }

    void apply(NetworkHandler& networkHandler) const override { networkHandler.onPlayNoteSound(*this); }
    [[nodiscard]] std::size_t size() const override { return 12; }
};

class ExplosionS2CPacket : public Packet {
public:
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    float radius = 0.0f;
    std::vector<Vec3i> affectedBlocks;

    void read(std::istream& input) override
    {
        x = packetio::readDoubleBE(input);
        y = packetio::readDoubleBE(input);
        z = packetio::readDoubleBE(input);
        radius = packetio::readFloatBE(input);
        const int count = packetio::readI32BE(input);
        const int originX = static_cast<int>(x);
        const int originY = static_cast<int>(y);
        const int originZ = static_cast<int>(z);
        affectedBlocks.clear();
        affectedBlocks.reserve(static_cast<std::size_t>(count));
        for (int i = 0; i < count; ++i) {
            affectedBlocks.emplace_back(
                originX + packetio::readI8(input),
                originY + packetio::readI8(input),
                originZ + packetio::readI8(input));
        }
    }

    void write(std::ostream& output) const override
    {
        packetio::writeDoubleBE(output, x);
        packetio::writeDoubleBE(output, y);
        packetio::writeDoubleBE(output, z);
        packetio::writeFloatBE(output, radius);
        packetio::writeI32BE(output, static_cast<std::int32_t>(affectedBlocks.size()));
        const int originX = static_cast<int>(x);
        const int originY = static_cast<int>(y);
        const int originZ = static_cast<int>(z);
        for (const Vec3i& blockPos : affectedBlocks) {
            packetio::writeI8(output, static_cast<std::int8_t>(blockPos.x - originX));
            packetio::writeI8(output, static_cast<std::int8_t>(blockPos.y - originY));
            packetio::writeI8(output, static_cast<std::int8_t>(blockPos.z - originZ));
        }
    }

    void apply(NetworkHandler& networkHandler) const override { networkHandler.onExplosion(*this); }
    [[nodiscard]] std::size_t size() const override { return 32U + affectedBlocks.size() * 3U; }
};

} // namespace net::minecraft
