#pragma once

namespace net::minecraft::entity::player {
enum class SleepAttemptResult {
    Ok,
    NotPossible,
    WrongDimension,
    MonstersNearby,
    TooFarAway
};
}  // namespace net::minecraft::entity::player
