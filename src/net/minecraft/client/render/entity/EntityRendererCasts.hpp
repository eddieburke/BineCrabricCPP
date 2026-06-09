#pragma once

// Type aliases for dynamic_cast in client::render::entity translation units.
// Avoids C++ name lookup clashes with the enclosing render::entity namespace.

#include "net/minecraft/entity/passive/ChickenEntity.hpp"
#include "net/minecraft/entity/passive/SheepEntity.hpp"
#include "net/minecraft/entity/passive/SquidEntity.hpp"
#include "net/minecraft/entity/projectile/ArrowEntity.hpp"
#include "net/minecraft/entity/vehicle/BoatEntity.hpp"
#include "net/minecraft/entity/mob/SlimeEntity.hpp"
#include "net/minecraft/entity/mob/SpiderEntity.hpp"

namespace net::minecraft::client::render::entity::casts {

using ChickenEntity = ::net::minecraft::entity::passive::ChickenEntity;
using SquidEntity = ::net::minecraft::entity::passive::SquidEntity;
using SheepEntity = ::net::minecraft::entity::passive::SheepEntity;
using ArrowEntity = ::net::minecraft::entity::projectile::ArrowEntity;
using BoatEntity = ::net::minecraft::entity::vehicle::BoatEntity;
using SlimeEntity = ::net::minecraft::entity::mob::SlimeEntity;
using SpiderEntity = ::net::minecraft::entity::mob::SpiderEntity;

} // namespace net::minecraft::client::render::entity::casts
