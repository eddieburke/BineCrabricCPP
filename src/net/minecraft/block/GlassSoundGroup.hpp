#pragma once

// Faithful port of net.minecraft.block.GlassSoundGroup (beta 1.7.3).

#include "net/minecraft/sound/BlockSoundGroup.hpp"

namespace net::minecraft::block {

class GlassSoundGroup : public net::minecraft::BlockSoundGroup {
public:
    GlassSoundGroup(std::string soundName, float volume, float pitch)
        : BlockSoundGroup(std::move(soundName), volume, pitch) {}

    [[nodiscard]] std::string getBreakSound() const override { return "random.glass"; }
};

} // namespace net::minecraft::block
