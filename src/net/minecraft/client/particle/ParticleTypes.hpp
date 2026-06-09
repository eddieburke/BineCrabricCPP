#pragma once

// M2 single-include contract: include this header at most once per file that
// needs net::minecraft particle type aliases. Do not sandwich includes between
// other headers — include ParticleTypes.hpp once in the include block.

#include "net/minecraft/client/particle/ParticleForward.hpp"

namespace net::minecraft {

using Particle         = particle::Particle;
using RainSplashParticle = particle::RainSplashParticle;
using BlockParticle    = particle::BlockParticle;
using FireSmokeParticle = particle::FireSmokeParticle;
using RedDustParticle  = particle::RedDustParticle;
using FlameParticle    = particle::FlameParticle;
using ItemParticle     = particle::ItemParticle;
using LavaEmberParticle = particle::LavaEmberParticle;
using FootstepParticle = particle::FootstepParticle;
using PickupParticle   = particle::PickupParticle;
using WaterSplashParticle = particle::WaterSplashParticle;
using WaterBubbleParticle = particle::WaterBubbleParticle;
using SnowParticle     = particle::SnowParticle;
using PortalParticle   = particle::PortalParticle;
using NoteParticle     = particle::NoteParticle;
using HeartParticle    = particle::HeartParticle;
using ExplosionParticle = particle::ExplosionParticle;

} // namespace net::minecraft
