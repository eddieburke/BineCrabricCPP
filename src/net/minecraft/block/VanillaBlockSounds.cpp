#include "net/minecraft/block/VanillaBlockSounds.hpp"

#include "net/minecraft/block/GlassSoundGroup.hpp"
#include "net/minecraft/block/SandSoundGroup.hpp"
#include "net/minecraft/sound/BlockSoundGroup.hpp"

namespace net::minecraft::block {

BlockSoundGroup& vanillaDefaultSound()
{
    static BlockSoundGroup sound("stone", 1.0f, 1.0f);
    return sound;
}

BlockSoundGroup& vanillaStoneSound()
{
    static BlockSoundGroup sound("stone", 1.0f, 1.0f);
    return sound;
}

BlockSoundGroup& vanillaWoodSound()
{
    static BlockSoundGroup sound("wood", 1.0f, 1.0f);
    return sound;
}

BlockSoundGroup& vanillaMetalSound()
{
    static BlockSoundGroup sound("stone", 1.0f, 1.5f);
    return sound;
}

BlockSoundGroup& vanillaDirtSound()
{
    static BlockSoundGroup sound("grass", 1.0f, 1.0f);
    return sound;
}

BlockSoundGroup& vanillaGravelSound()
{
    static BlockSoundGroup sound("gravel", 1.0f, 1.0f);
    return sound;
}

BlockSoundGroup& vanillaSandSound()
{
    static SandSoundGroup sound("sand", 1.0f, 1.0f);
    return sound;
}

BlockSoundGroup& vanillaWoolSound()
{
    static BlockSoundGroup sound("cloth", 1.0f, 1.0f);
    return sound;
}

BlockSoundGroup& vanillaGlassSound()
{
    static GlassSoundGroup sound("stone", 1.0f, 1.0f);
    return sound;
}

} // namespace net::minecraft::block
