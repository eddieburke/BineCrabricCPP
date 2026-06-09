#pragma once

namespace net::minecraft {

namespace entity::player {
class PlayerEntity;
}

class PlayerSaveHandler {
public:
    virtual ~PlayerSaveHandler() = default;

    virtual void savePlayerData(entity::player::PlayerEntity& player) = 0;
    virtual void loadPlayerData(entity::player::PlayerEntity& player) = 0;
};

} // namespace net::minecraft
