#pragma once

#include <string>
#include <thread>

namespace net::minecraft::client {
class Minecraft;
}

namespace net::minecraft::client::util {

/// Background thread that sleeps until `Minecraft::running` is false (Java Timer hack).
/// Anchor: Minecraft.cpp L108–118.
class TimerHackThread : public std::thread {
public:
    explicit TimerHackThread(Minecraft* client, const std::string& name);
};

} // namespace net::minecraft::client::util
