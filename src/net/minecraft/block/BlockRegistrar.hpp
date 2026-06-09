#pragma once

// Common includes for block self-registration translation units. Pulls in World
// so block headers with inline World-dependent methods compile in thin .cpp files.
#include "net/minecraft/block/VanillaBlockSounds.hpp"
#include "net/minecraft/registry/VanillaRegistry.hpp"
#include "net/minecraft/world/World.hpp"
