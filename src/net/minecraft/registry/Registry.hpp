#pragma once

#include "net/minecraft/recipe/CraftingRecipeManager.hpp"
#include "net/minecraft/recipe/SmeltingRecipeManager.hpp"

#include <vector>

namespace net::minecraft::registry {

inline constexpr int kBiomeRegistrarPriority = -1000;
inline constexpr int kBlockRegistrarBase = 0;
inline constexpr int kFinalizeBlockRegistryPriority = 500;
inline constexpr int kSpecialBlockItemRegistrarBase = 2500;
inline constexpr int kGenericBlockItemRegistrarPriority = 2600;
inline constexpr int kSmeltingRecipeRegistrarBase = 2650;
inline constexpr int kItemRegistrarBase = 1000;
inline constexpr int kCraftingRecipeRegistrarBase = 2700;
inline constexpr int kCraftingRecipeFinalizePriority = 2999;
inline constexpr int kEntityRegistrarBase = 30000;
inline constexpr int kClientRendererRegistrarBase = 40000;

inline constexpr int blockItemRegistrarPriority(int /*blockId*/)
{
    return kSpecialBlockItemRegistrarBase;
}

#define MINECRAFT_REGISTER(fn, priority) \
    namespace { \
        struct _McReg_##fn { static void registerClass() { fn(); } }; \
        static ::net::minecraft::registry::RegisterCustom<_McReg_##fn> \
            _mcReg_##fn(priority); \
    }

class Registry {
public:
    static void addBlock(int id, void (*init)());
    static void addItem(int id, void (*init)());
    static void addEntity(int id, void (*init)());
    static void addCustom(int priority, void (*init)());

    // Called once at startup to execute everything in order
    static void bootstrap();
};

namespace detail {

template <typename T>
constexpr bool hasRegisterRecipes =
    requires { T::registerRecipes(std::declval<net::minecraft::recipe::CraftingRecipeManager&>()); };

template <typename T>
constexpr bool hasRegisterSmeltingRecipes = requires { T::registerSmeltingRecipes(); };

template <typename T>
    requires hasRegisterRecipes<T>
void registerCraftingRecipes()
{
    T::registerRecipes(net::minecraft::recipe::CraftingRecipeManager::getInstance());
}

template <typename T>
    requires hasRegisterSmeltingRecipes<T>
void registerSmeltingRecipes()
{
    T::registerSmeltingRecipes();
}

template <typename T>
void enqueueCraftingRecipes(int priorityOffset)
{
    if constexpr (hasRegisterRecipes<T>) {
        Registry::addCustom(kCraftingRecipeRegistrarBase + priorityOffset, registerCraftingRecipes<T>);
    }
}

template <typename T>
void enqueueSmeltingRecipes(int priorityOffset)
{
    if constexpr (hasRegisterSmeltingRecipes<T>) {
        Registry::addCustom(kSmeltingRecipeRegistrarBase + priorityOffset, registerSmeltingRecipes<T>);
    }
}

// Blocks with custom block items define `static void registerBlockItems()`;
// it runs after all blocks exist but before the generic BlockItem pass, so the
// generic pass skips the slots it fills. Self-contained: no central table.
template <typename T>
constexpr bool hasRegisterBlockItems = requires { T::registerBlockItems(); };

template <typename T>
void enqueueBlockItems(int blockId)
{
    if constexpr (hasRegisterBlockItems<T>) {
        Registry::addCustom(kSpecialBlockItemRegistrarBase + blockId, &T::registerBlockItems);
    }
}

} // namespace detail

// Trait-based default ctor when T::kRegisters; legacy (int id) ctor otherwise.
template <typename T>
struct RegisterBlock {
    RegisterBlock() requires (requires { T::kRegisters; } && T::kRegisters)
    {
        Registry::addBlock(T::kBlockId, T::registerClass);
        detail::enqueueBlockItems<T>(T::kBlockId);
        detail::enqueueCraftingRecipes<T>(T::kBlockId);
        detail::enqueueSmeltingRecipes<T>(T::kBlockId);
    }
    explicit RegisterBlock(int id) requires (!requires { T::kRegisters; } || !T::kRegisters)
    {
        Registry::addBlock(id, T::registerClass);
        detail::enqueueBlockItems<T>(id);
        detail::enqueueCraftingRecipes<T>(id);
        detail::enqueueSmeltingRecipes<T>(id);
    }
};

template <typename T>
struct RegisterItem {
    explicit RegisterItem(int rawId)
    {
        Registry::addItem(rawId, T::registerClass);
        detail::enqueueCraftingRecipes<T>(rawId);
        detail::enqueueSmeltingRecipes<T>(rawId);
    }
};

template <typename T>
struct RegisterEntity {
    explicit RegisterEntity(int rawId)
    {
        Registry::addEntity(rawId, T::registerClass);
    }
};

template <typename T>
struct RegisterCustom {
    RegisterCustom(int priority) { Registry::addCustom(priority, T::registerClass); }
};

} // namespace net::minecraft::registry
