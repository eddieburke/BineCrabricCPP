#pragma once



#include "net/minecraft/entity/EntityRegistry.hpp"

#include "net/minecraft/mod/ModLifecycle.hpp"

#include "net/minecraft/recipe/CraftingRecipeManager.hpp"

#include "net/minecraft/recipe/SmeltingRecipeManager.hpp"

#include "net/minecraft/world/World.hpp"



#include <memory>

#include <typeindex>

#include <vector>



namespace net::minecraft::registry {



inline constexpr int kGenericBlockItemOrder = 1000;

inline constexpr int kCraftingRecipeFinalizeOrder = 10000;



class Registry {

public:

    static void enqueue(mod::LifecyclePhase phase, int order, void (*init)());



    static void addBlock(int id, void (*init)());

    static void addItem(int id, void (*init)());

    static void addEntity(int rawId, void (*init)());



    static void bootstrap();

    [[nodiscard]] static bool isBootstrapped();

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

void enqueueCraftingRecipes(int orderOffset)

{

    if constexpr (hasRegisterRecipes<T>) {

        Registry::enqueue(mod::LifecyclePhase::CraftingRecipeRegistration, orderOffset, registerCraftingRecipes<T>);

    }

}



template <typename T>

void enqueueSmeltingRecipes(int orderOffset)

{

    if constexpr (hasRegisterSmeltingRecipes<T>) {

        Registry::enqueue(mod::LifecyclePhase::SmeltingRecipeRegistration, orderOffset, registerSmeltingRecipes<T>);

    }

}



template <typename T>

constexpr bool hasRegisterBlockItems = requires { T::registerBlockItems(); };



template <typename T>

void enqueueBlockItems(int blockId)

{

    if constexpr (hasRegisterBlockItems<T>) {

        Registry::enqueue(mod::LifecyclePhase::BlockItemRegistration, blockId, &T::registerBlockItems);

    }

}



template<typename EntityType>

void registerVanillaEntity()

{

    entity::EntityRegistry::registerType(

        std::type_index(typeid(EntityType)),

        EntityType::kEntityName,

        EntityType::kEntityId,

        [](World* world) -> std::unique_ptr<entity::Entity> {

            return std::make_unique<EntityType>(world);

        });

}



template<typename EntityType>

void bootstrapEntity()

{

    registerVanillaEntity<EntityType>();

}



} // namespace detail



template <typename T>

struct RegisterBlock {

    RegisterBlock()

    {

        Registry::addBlock(T::kBlockId, T::registerClass);

        detail::enqueueBlockItems<T>(T::kBlockId);

        detail::enqueueCraftingRecipes<T>(T::kBlockId);

        detail::enqueueSmeltingRecipes<T>(T::kBlockId);

    }

};



template <typename T>

struct RegisterItem {

    RegisterItem()

    {

        Registry::addItem(T::kRawId, T::registerClass);

        detail::enqueueCraftingRecipes<T>(T::kRawId);

        detail::enqueueSmeltingRecipes<T>(T::kRawId);

    }

};



template <typename T>

struct RegisterEntity {

    RegisterEntity() { Registry::addEntity(T::kEntityId, detail::bootstrapEntity<T>); }

};



template <typename T>

struct RegisterPhase {

    RegisterPhase(mod::LifecyclePhase phase, int order) { Registry::enqueue(phase, order, T::registerClass); }

};



} // namespace net::minecraft::registry

// Self-registration: drop one of these at the end of the type's .cpp. The
// anonymous-namespace static runs at program init and enqueues the type into
// its bootstrap phase, ordered by the type's id constant — no central list.
#define MC_REGISTER_BLOCK(T) \
    namespace { ::net::minecraft::registry::RegisterBlock<T> mcAutoReg_##T; }
#define MC_REGISTER_ITEM(T) \
    namespace { ::net::minecraft::registry::RegisterItem<T> mcAutoReg_##T; }
#define MC_REGISTER_ENTITY(T) \
    namespace { ::net::minecraft::registry::RegisterEntity<T> mcAutoReg_##T; }

