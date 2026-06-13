#include "net/minecraft/client/render/chunk/GlListPool.hpp"

#include "net/minecraft/client/gl/GL11.hpp"

namespace net::minecraft::client::render::chunk {

GlListPool::~GlListPool()
{
    releaseAll();
}

bool GlListPool::reserveBlock()
{
    const int base = gl::GL11::glGenLists(kListsPerBlock);
    if (base == 0) {
        return false;
    }
    blockBases_.push_back(base);
    cursorBase_ = base;
    cursorEnd_ = base + kListsPerBlock;
    return true;
}

int GlListPool::acquirePair()
{
    if (!freeBases_.empty()) {
        const int base = freeBases_.back();
        freeBases_.pop_back();
        return base;
    }
    if (cursorBase_ >= cursorEnd_ && !reserveBlock()) {
        return -1;
    }
    const int base = cursorBase_;
    cursorBase_ += 2;
    return base;
}

void GlListPool::releasePair(int base)
{
    if (base >= 0) {
        freeBases_.push_back(base);
    }
}

void GlListPool::releaseAll()
{
    for (const int base : blockBases_) {
        gl::GL11::glDeleteLists(base, kListsPerBlock);
    }
    blockBases_.clear();
    freeBases_.clear();
    cursorBase_ = 0;
    cursorEnd_ = 0;
}

} // namespace net::minecraft::client::render::chunk
