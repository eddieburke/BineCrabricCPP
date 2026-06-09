#pragma once

// M1 single-include contract: include this header at most once per file that
// needs net::minecraft block type aliases. Do not sandwich includes between
// other headers — include BlockTypes.hpp once in the include block.

#include "net/minecraft/block/BlockForward.hpp"

namespace net::minecraft {

using Block = block::Block;
using BlockDefinition = block::BlockDefinition;
using BlockRegistry = block::BlockRegistry;

inline void initializeBlocks()
{
    block::initializeBlocks();
}

} // namespace net::minecraft
