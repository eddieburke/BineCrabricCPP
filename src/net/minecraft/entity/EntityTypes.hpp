#pragma once
// M1 single-include contract: include this header at most once per file that
// needs net::minecraft entity type aliases. Do not sandwich includes between
// other headers — include EntityTypes.hpp once in the include block.
#include <optional>
#include <typeindex>

#include "net/minecraft/util/math/MathConstants.hpp"

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
}  // namespace net::minecraft::entity

namespace net::minecraft::entity::data {
class DataTracker;
class DataTrackerEntry;
}  // namespace net::minecraft::entity::data

namespace net::minecraft::entity::player {
class PlayerEntity;
class PlayerInventory;
}  // namespace net::minecraft::entity::player

namespace net::minecraft {
[[nodiscard]] std::optional<std::type_index> entitySupertype(std::type_index type);
using Entity = entity::Entity;
using LivingEntity = entity::LivingEntity;
using MobEntity = entity::MobEntity;
using FlyingEntity = entity::FlyingEntity;
using WaterCreatureEntity = entity::WaterCreatureEntity;
using ItemEntity = entity::ItemEntity;
using FallingBlockEntity = entity::FallingBlockEntity;
using LightningEntity = entity::LightningEntity;
using TntEntity = entity::TntEntity;
using EntityRegistry = entity::EntityRegistry;
using Monster = entity::Monster;
using DataTracker = entity::data::DataTracker;
using DataTrackerEntry = entity::data::DataTrackerEntry;
using PlayerEntity = entity::player::PlayerEntity;
using PlayerInventory = entity::player::PlayerInventory;
using util::math::kPiF;
}  // namespace net::minecraft
