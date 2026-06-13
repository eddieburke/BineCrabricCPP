#pragma once

#include <vector>

namespace net::minecraft::client::render::chunk {

// Pool of GL display-list ID pairs (solid + translucent) for render sections.
// IDs are reserved in fixed-size blocks via glGenLists and recycled when a
// section is freed, so the live ID range scales with the number of resident
// sections rather than with render distance. This removes the old fixed
// 64^3*2 reservation that could overflow into GUI/text list IDs at large
// render distances.
//
// All methods touch GL and must run on the render (main) thread.
class GlListPool {
public:
    GlListPool() = default;
    ~GlListPool();

    GlListPool(const GlListPool&) = delete;
    GlListPool& operator=(const GlListPool&) = delete;

    // Base id of a fresh pair {base, base+1}, or -1 if GL allocation failed.
    [[nodiscard]] int acquirePair();

    // Return a pair previously handed out by acquirePair for reuse.
    void releasePair(int base);

    // Delete every reserved block. Invalidates all handed-out ids; callers must
    // have stopped using them. Safe to call with nothing reserved.
    void releaseAll();

private:
    // 4096 pairs => one glGenLists(8192) call per block.
    static constexpr int kPairsPerBlock = 4096;
    static constexpr int kListsPerBlock = kPairsPerBlock * 2;

    bool reserveBlock();

    std::vector<int> freeBases_;   // recycled pair bases
    std::vector<int> blockBases_;  // base id of each glGenLists block (for delete)
    int cursorBase_ = 0;           // next unused base within the current block
    int cursorEnd_ = 0;            // one past the last base in the current block
};

} // namespace net::minecraft::client::render::chunk
