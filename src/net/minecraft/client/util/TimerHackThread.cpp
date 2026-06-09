#include "net/minecraft/client/util/TimerHackThread.hpp"

#include "net/minecraft/client/Minecraft.hpp"

#include <chrono>

namespace net::minecraft::client::util {

TimerHackThread::TimerHackThread(Minecraft* client, const std::string& name)
    : std::thread([client, name]() {
        (void)name;
        while (client->running.load()) {
            std::this_thread::sleep_for(std::chrono::hours(24 * 365));
        }
    })
{
}

} // namespace net::minecraft::client::util
