#include "net/minecraft/world/events/WorldEvents.hpp"

#include <algorithm>

#include "net/minecraft/entity/player/PlayerEntity.hpp"
#include "net/minecraft/world/World.hpp"

namespace net::minecraft {
WorldEvents::WorldEvents(World& world) : world_(world) {
}

void WorldEvents::addEventListener(GameEventListener* listener) {
    if (listener == nullptr) {
        return;
    }
    for (GameEventListener* existing : eventListeners_) {
        if (existing == listener) {
            return;
        }
    }
    eventListeners_.push_back(listener);
}

void WorldEvents::removeEventListener(GameEventListener* listener) {
    eventListeners_.erase(std::remove(eventListeners_.begin(), eventListeners_.end(), listener), eventListeners_.end());
}

void WorldEvents::blockUpdateEvent(int x, int y, int z) {
    for (GameEventListener* listener : eventListeners_) {
        if (listener != nullptr) {
            listener->blockUpdate(x, y, z);
        }
    }
}

void WorldEvents::setBlockDirty(int x, int y, int z) {
    for (GameEventListener* listener : eventListeners_) {
        if (listener != nullptr) {
            listener->setBlocksDirty(x, y, z, x, y, z);
        }
    }
}

void WorldEvents::notifyEntityAdded(Entity* entity) {
    for (GameEventListener* listener : eventListeners_) {
        if (listener != nullptr) {
            listener->notifyEntityAdded(entity);
        }
    }
}

void WorldEvents::notifyEntityRemoved(Entity* entity) {
    for (GameEventListener* listener : eventListeners_) {
        if (listener != nullptr) {
            listener->notifyEntityRemoved(entity);
        }
    }
}

void WorldEvents::notifyEntityPickup(Entity* entity, PlayerEntity* collector) {
    for (GameEventListener* listener : eventListeners_) {
        if (listener != nullptr) {
            listener->onEntityPickup(entity, collector);
        }
    }
}

void WorldEvents::notifyAmbientDarknessChanged() {
    for (GameEventListener* listener : eventListeners_) {
        if (listener != nullptr) {
            listener->notifyAmbientDarknessChanged();
        }
    }
}

void WorldEvents::playSound(double x, double y, double z, const std::string& name, float volume, float pitch) {
    for (GameEventListener* listener : eventListeners_) {
        if (listener != nullptr) {
            listener->playSound(name, x, y, z, volume, pitch);
        }
    }
}

void WorldEvents::playSound(Entity* source, const std::string& name, float volume, float pitch) {
    if (source == nullptr) {
        playSound(0.0, 0.0, 0.0, name, volume, pitch);
        return;
    }
    playSound(source->x, source->y - static_cast<double>(source->standingEyeHeight), source->z, name, volume, pitch);
}

void WorldEvents::playSound(PlayerEntity* player, const std::string& name, float volume, float pitch) {
    if (player == nullptr) {
        playSound(0.0, 0.0, 0.0, name, volume, pitch);
        return;
    }
    playSound(player->x, player->y, player->z, name, volume, pitch);
}

void WorldEvents::playStreaming(const std::string& name, int x, int y, int z) {
    for (GameEventListener* listener : eventListeners_) {
        if (listener != nullptr) {
            listener->playStreaming(name, x, y, z);
        }
    }
}

void WorldEvents::blockBreakParticles(int x, int y, int z, int blockId, int blockMeta) {
    for (GameEventListener* listener : eventListeners_) {
        if (listener != nullptr) {
            listener->blockBreakParticles(x, y, z, blockId, blockMeta);
        }
    }
}

void WorldEvents::setBlocksDirty(int minX, int minY, int minZ, int maxX, int maxY, int maxZ) {
    if (minY > maxY) {
        std::swap(minY, maxY);
    }
    for (GameEventListener* listener : eventListeners_) {
        if (listener != nullptr) {
            listener->setBlocksDirty(minX, minY, minZ, maxX, maxY, maxZ);
        }
    }
}

void WorldEvents::setBlocksDirtyColumn(int x, int z, int minY, int maxY) {
    setBlocksDirty(x, minY, z, x, maxY, z);
}

void WorldEvents::updateBlockEntity(int x, int y, int z, block::entity::BlockEntity* blockEntity) {
    for (GameEventListener* listener : eventListeners_) {
        if (listener != nullptr) {
            listener->updateBlockEntity(x, y, z, blockEntity);
        }
    }
}

void WorldEvents::addParticle(
    const std::string& name, double px, double py, double pz, double vx, double vy, double vz) {
    for (GameEventListener* listener : eventListeners_) {
        if (listener != nullptr) {
            listener->addParticle(name, px, py, pz, vx, vy, vz);
        }
    }
}

void WorldEvents::worldEvent(PlayerEntity* player, int event, int x, int y, int z, int data) {
    for (GameEventListener* listener : eventListeners_) {
        if (listener != nullptr) {
            listener->worldEvent(player, event, x, y, z, data);
        }
    }
}
}  // namespace net::minecraft
