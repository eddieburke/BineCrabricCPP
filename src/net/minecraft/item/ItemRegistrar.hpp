#pragma once

// Common includes for item self-registration translation units. World must be
// first so item headers with World-dependent methods compile in thin .cpp files.
#include "net/minecraft/block/Block.hpp"
#include "net/minecraft/registry/VanillaRegistry.hpp"
#include "net/minecraft/world/World.hpp"
