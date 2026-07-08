#pragma once

// M1 single-include contract: include this header at most once per file that
// needs net::minecraft block type aliases. Do not sandwich includes between
// other headers — include BlockTypes.hpp once in the include block.
namespace net::minecraft::block {
class Block;
void initializeBlocks();
}  // namespace net::minecraft::block

namespace net::minecraft {
using Block = block::Block;

inline void initializeBlocks() {
    block::initializeBlocks();
}
}  // namespace net::minecraft
