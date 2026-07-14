#pragma once
#include <cstdint>
struct lua_State;
namespace net::minecraft::block::entity {
class BlockEntity;
}
namespace net::minecraft::mod::runtime {
void installTileEntityApi(lua_State* state);
void pushTileEntityHandle(lua_State* state, net::minecraft::block::entity::BlockEntity* entity);
[[nodiscard]] int tileEntityAnimationFrame(const net::minecraft::block::entity::BlockEntity* entity);
[[nodiscard]] float tileEntityAnimationSpeed(const net::minecraft::block::entity::BlockEntity* entity);
[[nodiscard]] std::int64_t tileEntityAnimTick(const net::minecraft::block::entity::BlockEntity* entity);
void setTileEntityAnimationSpeed(const net::minecraft::block::entity::BlockEntity* entity, float speed);
void tickTileEntityAnimation(const net::minecraft::block::entity::BlockEntity* entity);
void clearTileEntityAnimation(const net::minecraft::block::entity::BlockEntity* entity);
} // namespace net::minecraft::mod::runtime
