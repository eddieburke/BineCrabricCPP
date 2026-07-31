#pragma once
// M1 single-include contract: include this header at most once per file that
// needs net::minecraft block type aliases. Do not sandwich includes between
// other headers — include BlockTypes.hpp once in the include block.
namespace net::minecraft::block {
class Block;
} // namespace net::minecraft::block
namespace net::minecraft {
using Block = block::Block;
} // namespace net::minecraft
