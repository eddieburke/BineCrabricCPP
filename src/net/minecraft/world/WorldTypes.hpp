#pragma once

// M1 single-include contract: include this header at most once per file that
// needs net::minecraft world type forward declarations. Do not sandwich includes
// between other headers — include WorldTypes.hpp once in the include block.
//
// Forward-only hub — consumers needing class definitions include World.hpp directly.
// All world types (World, ClientWorld, ServerWorld, Chunk, ChunkSource, BlockView,
// IBlockWorld, IEntityWorld, Explosion, BiomeSource, Dimension) live directly in
// net::minecraft — no sub-namespace aliasing required.

#include "net/minecraft/world/WorldForward.hpp"
