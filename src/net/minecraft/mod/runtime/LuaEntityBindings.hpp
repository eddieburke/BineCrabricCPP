#pragma once
#include "net/minecraft/entity/LivingEntity.hpp"
#include "net/minecraft/mod/runtime/LuaDirectHooks.hpp"
#include "net/minecraft/mod/runtime/ModHost.hpp"
struct lua_State;
namespace net::minecraft::mod::runtime {
void installEntityApi(lua_State* state, ModHost::LoadedLuaMod& mod);
void applyRegisteredPoseHooks(const net::minecraft::entity::LivingEntity& entity,
                              float tickDelta,
                              net::minecraft::mod::EntityRenderPose& pose);
} // namespace net::minecraft::mod::runtime
