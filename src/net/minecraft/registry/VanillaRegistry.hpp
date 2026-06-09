#pragma once

#include "net/minecraft/registry/AutoRegistry.hpp"

namespace net::minecraft::registry {

struct VanillaRegistryTag {};

// Priority bands keep blocks, items, block items, and bootstrap hooks ordered.
inline constexpr int blockRegistrarPriority(int blockId)
{
    return blockId;
}

inline constexpr int itemRegistrarPriority(int itemRawId)
{
    return 1000 + itemRawId;
}

inline constexpr int blockItemRegistrarPriority(int blockId)
{
    return 10000 + blockId;
}

inline constexpr int kBiomeRegistrarPriority = 500;
inline constexpr int kFinalizeBlockRegistryPriority = 900;
inline constexpr int kGenericBlockItemRegistrarPriority = 20000;
inline constexpr int kEntityBootstrapPriority = 30000;

void runVanillaBootstrap();

} // namespace net::minecraft::registry

// Unified static registration for blocks, items, and bootstrap hooks.
#define MINECRAFT_REGISTER(FunctionName, Priority)                                                   \
    const ::net::minecraft::registry::AutoRegistrar<::net::minecraft::registry::VanillaRegistryTag>    \
        _minecraftRegistrar_##FunctionName(FunctionName, Priority);

#define MINECRAFT_REGISTER_BLOCK(FunctionName, BlockId)                                              \
    MINECRAFT_REGISTER(FunctionName, ::net::minecraft::registry::blockRegistrarPriority(BlockId))

#define MINECRAFT_REGISTER_ITEM(FunctionName, ItemRawId)                                             \
    MINECRAFT_REGISTER(FunctionName, ::net::minecraft::registry::itemRegistrarPriority(ItemRawId))

#define MINECRAFT_REGISTER_BLOCK_ITEM(FunctionName, BlockId)                                       \
    MINECRAFT_REGISTER(FunctionName, ::net::minecraft::registry::blockItemRegistrarPriority(BlockId))
