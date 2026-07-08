#pragma once
// Faithful 1:1 port of net.minecraft.sound.BlockSoundGroup (beta 1.7.3).
// stepSoundName drives footstep/mining audio; breakSound overrides final-break
// audio for glass/sand (Java's GlassSoundGroup / SandSoundGroup subclasses).
#include <string>

namespace net::minecraft {
class BlockSoundGroup {
   public:
    std::string stepSoundName;
    float volume;
    float pitch;
    std::string breakSound;

    BlockSoundGroup(std::string stepSoundName, float volume, float pitch, std::string breakSound = "")
        : stepSoundName(std::move(stepSoundName)), volume(volume), pitch(pitch), breakSound(std::move(breakSound)) {
    }

    [[nodiscard]] float getVolume() const {
        return volume;
    }

    [[nodiscard]] float getPitch() const {
        return pitch;
    }

    [[nodiscard]] std::string getStepSound() const {
        return "step." + stepSoundName;
    }

    [[nodiscard]] std::string getMiningSound() const {
        return getStepSound();
    }

    [[nodiscard]] std::string getBreakSound() const {
        return breakSound.empty() ? getStepSound() : breakSound;
    }
};
}  // namespace net::minecraft
