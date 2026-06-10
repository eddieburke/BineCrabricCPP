#include "net/minecraft/block/Block.hpp"

#include "net/minecraft/registry/Registry.hpp"

namespace net::minecraft::block {

struct BlockRegistryFinalizer {
    static void registerClass() { finalizeBlockRegistryProperties(); }
};

static registry::RegisterCustom<BlockRegistryFinalizer> s_reg(registry::kFinalizeBlockRegistryPriority);

} // namespace net::minecraft::block
