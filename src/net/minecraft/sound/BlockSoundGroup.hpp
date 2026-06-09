#pragma once

// Faithful 1:1 port of net.minecraft.sound.BlockSoundGroup (beta 1.7.3).
// Virtual getBreakSound so GlassSoundGroup / SandSoundGroup can override.

#include <string>

namespace net::minecraft {

class BlockSoundGroup {
public:
    std::string soundName;
    float volume;
    float pitch;

    BlockSoundGroup(std::string soundName, float volume, float pitch)
        : soundName(std::move(soundName)), volume(volume), pitch(pitch) {}

    virtual ~BlockSoundGroup() = default;

    [[nodiscard]] float getVolume() const { return volume; }
    [[nodiscard]] float getPitch() const { return pitch; }

    [[nodiscard]] virtual std::string getBreakSound() const { return "step." + soundName; }
    [[nodiscard]] std::string getSound() const { return "step." + soundName; }
};

} // namespace net::minecraft
