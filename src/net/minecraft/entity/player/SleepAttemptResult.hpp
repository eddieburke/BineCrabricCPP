#pragma once

namespace net::minecraft::entity::player {

enum class SleepAttemptResult {
    Ok,
    NotPossible,
    WrongDimension,
    MonstersNearby,
    TooFarAway
};

// Compatibility aliases for callers not yet migrated (e.g. block/BedBlock.cpp).
inline constexpr SleepAttemptResult field2660 = SleepAttemptResult::Ok;
inline constexpr SleepAttemptResult field2661 = SleepAttemptResult::NotPossible;
inline constexpr SleepAttemptResult field2662 = SleepAttemptResult::WrongDimension;
inline constexpr SleepAttemptResult field2663 = SleepAttemptResult::MonstersNearby;
inline constexpr SleepAttemptResult field2664 = SleepAttemptResult::TooFarAway;

} // namespace net::minecraft::entity::player
