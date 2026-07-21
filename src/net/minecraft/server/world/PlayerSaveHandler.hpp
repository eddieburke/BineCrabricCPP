#pragma once
namespace net::minecraft::entity::player {
class PlayerEntity;
}
namespace net::minecraft::server::world {
class PlayerSaveHandler {
 public:
 virtual ~PlayerSaveHandler() = default;
 virtual void savePlayerData(::net::minecraft::entity::player::PlayerEntity& player) = 0;
 virtual void loadPlayerData(::net::minecraft::entity::player::PlayerEntity& player) = 0;
};
} // namespace net::minecraft::server::world
