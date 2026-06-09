#pragma once

// Forward declarations only — no using aliases.
// Use EntityTypes.hpp when net::minecraft::* aliases are required.

namespace net::minecraft::entity {
class Entity;
class LivingEntity;
class MobEntity;
class FlyingEntity;
class WaterCreatureEntity;
class ItemEntity;
class FallingBlockEntity;
class LightningEntity;
class TntEntity;
class EntityRegistry;
class Monster;
} // namespace net::minecraft::entity

namespace net::minecraft::entity::data {
class DataTracker;
class DataTrackerEntry;
} // namespace net::minecraft::entity::data

namespace net::minecraft::entity::player {
class PlayerEntity;
class PlayerInventory;
} // namespace net::minecraft::entity::player
