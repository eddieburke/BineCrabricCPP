#pragma once

// Faithful port of net.minecraft.block.SandSoundGroup (beta 1.7.3).

#include "net/minecraft/sound/BlockSoundGroup.hpp"

namespace net::minecraft::block {

class SandSoundGroup : public net::minecraft::BlockSoundGroup {
public:
    SandSoundGroup(std::string soundName, float volume, float pitch)
        : BlockSoundGroup(std::move(soundName), volume, pitch) {}

    [[nodiscard]] std::string getBreakSound() const override { return "step.gravel"; }
};

} // namespace net::minecraft::block
