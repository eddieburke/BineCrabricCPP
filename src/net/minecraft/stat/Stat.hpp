#pragma once

// Legacy compatibility alias: native gameplay uses stat::StatId in Stats.hpp.

namespace net::minecraft::stat {

struct Stat {
    bool localOnly = false;
};

} // namespace net::minecraft::stat
