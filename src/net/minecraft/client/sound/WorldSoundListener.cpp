#include "net/minecraft/client/sound/WorldSoundListener.hpp"

#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/option/ResolvedRenderOptions.hpp"
#include "net/minecraft/entity/LivingEntity.hpp"
#include "net/minecraft/util/math/MathHelper.hpp"
#include "net/minecraft/world/World.hpp"
#include "net/minecraft/world/biome/source/BiomeSource.hpp"

namespace net::minecraft::client::sound {
WorldSoundListener::WorldSoundListener(Minecraft* client) : client_(client) {
}

void WorldSoundListener::attach(net::minecraft::World* world) {
    if (world != nullptr) {
        world->addEventListener(this);
    }
}

void WorldSoundListener::detach(net::minecraft::World* world) {
    if (world != nullptr) {
        world->removeEventListener(this);
    }
}

void WorldSoundListener::playAt(const std::string& sound, double x, double y, double z, float volume, float pitch) {
    if (client_ == nullptr || client_->camera == nullptr) {
        return;
    }
    float range = 16.0f;
    if (volume > 1.0f) {
        range *= volume;
    }
    if (client_->camera->getSquaredDistance(x, y, z) > static_cast<double>(range * range)) {
        return;
    }
    client_->audio.playAt(sound, static_cast<float>(x), static_cast<float>(y), static_cast<float>(z), volume, pitch);
}

void WorldSoundListener::playSound(const std::string& sound, double x, double y, double z, float volume, float pitch) {
    playAt(sound, x, y, z, volume, pitch);
}

void WorldSoundListener::playStreaming(const std::string& stream, int x, int y, int z) {
    if (client_ == nullptr) {
        return;
    }
    if (!stream.empty()) {
        client_->inGameHud.setRecordPlayingOverlay("C418 - " + stream);
    }
    client_->audio.playRecord(stream, static_cast<float>(x), static_cast<float>(y), static_cast<float>(z), 1.0f);
}

void WorldSoundListener::tickWeather(Minecraft& client) {
    if (client.world == nullptr || client.camera == nullptr) {
        return;
    }
    float rain = option::rainGradient(option::resolve(client.options), client.world, 1.0f);
    if (!client.options.fancyGraphics) {
        rain /= 2.0f;
    }
    if (rain == 0.0f) {
        return;
    }
    const auto* living = dynamic_cast<const LivingEntity*>(client.camera);
    if (living == nullptr) {
        return;
    }
    World* world = client.world;
    const int baseX = MathHelper::floor(living->x);
    const int baseY = MathHelper::floor(living->y);
    const int baseZ = MathHelper::floor(living->z);
    constexpr int radius = 10;
    JavaRandom random;
    random.setSeed(static_cast<std::uint64_t>(client.ticksPlayed) * 312987231ULL);
    BiomeSource* biomeSource = world->getBiomeSource();
    double soundX = 0.0;
    double soundY = 0.0;
    double soundZ = 0.0;
    int rainSamples = 0;
    const int sampleCount = static_cast<int>(100.0f * rain * rain);
    for (int i = 0; i < sampleCount; ++i) {
        const int px = baseX + random.nextInt(radius) - random.nextInt(radius);
        const int pz = baseZ + random.nextInt(radius) - random.nextInt(radius);
        const int topY = world->getTopSolidBlockY(px, pz);
        if (topY < 0) {
            continue;
        }
        const int belowId = world->getBlockId(px, topY - 1, pz);
        if (topY > baseY + radius || topY < baseY - radius) {
            continue;
        }
        if (biomeSource == nullptr || !biomeSource->getBiome(px, pz).canRain()) {
            continue;
        }
        if (belowId <= 0) {
            continue;
        }
        Block* block = Block::BLOCKS[static_cast<std::size_t>(belowId)];
        if (block == nullptr) {
            continue;
        }
        const float rx = random.nextFloat();
        const float rz = random.nextFloat();
        const double py = static_cast<double>(static_cast<float>(topY) + 0.1f) - block->minY;
        if (random.nextInt(++rainSamples) == 0) {
            soundX = static_cast<float>(px) + rx;
            soundY = py;
            soundZ = static_cast<float>(pz) + rz;
        }
    }
    if (rainSamples > 0 && random.nextInt(3) < rainSoundCounter_++) {
        rainSoundCounter_ = 0;
        if (soundY > living->y + 1.0 &&
            world->getTopSolidBlockY(MathHelper::floor(living->x), MathHelper::floor(living->z)) >
                MathHelper::floor(living->y)) {
            world->playSound(soundX, soundY, soundZ, "ambient.weather.rain", 0.1f, 0.5f);
        } else {
            world->playSound(soundX, soundY, soundZ, "ambient.weather.rain", 0.2f, 1.0f);
        }
    }
}
}  // namespace net::minecraft::client::sound
