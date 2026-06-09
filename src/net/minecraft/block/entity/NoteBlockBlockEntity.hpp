#pragma once

#include "net/minecraft/block/entity/BlockEntity.hpp"

#include <cstdint>

namespace net::minecraft {
class World;
}

namespace net::minecraft::block::entity {

class NoteBlockBlockEntity : public BlockEntity {
public:
    void writeNbt(NbtCompound& nbt) const override;
    void readNbt(const NbtCompound& nbt) override;
    void cycleNote();
    void playNote(World* world, int noteX, int noteY, int noteZ);
    [[nodiscard]] std::string id() const override
    {
        return "Music";
    }

    std::int8_t note = 0;
    bool powered = false;
};

} // namespace net::minecraft::block::entity
