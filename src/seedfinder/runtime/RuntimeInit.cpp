#include "seedfinder/runtime/RuntimeInit.hpp"

#include "net/minecraft/block/Block.hpp"

namespace seedfinder::runtime {

void initialize()
{
    net::minecraft::block::initializeBlocks();
}

void shutdown()
{
    // Reserved for future thread-pool / scratch teardown. Registries are process-lifetime.
}

} // namespace seedfinder::runtime

extern "C" {

void seedfinder_init(void)
{
    seedfinder::runtime::initialize();
}

void seedfinder_shutdown(void)
{
    seedfinder::runtime::shutdown();
}

} // extern "C"
