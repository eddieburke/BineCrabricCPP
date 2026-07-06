#include "net/minecraft/client/util/TimerHackThread.hpp"
#include "net/minecraft/client/Minecraft.hpp"
#include <chrono>
namespace net::minecraft::client::util {
TimerHackThread::TimerHackThread(Minecraft* client, const std::string& name)
    : thread_([client, name](const std::stop_token& stop) {
        (void)name;
        while(!stop.stop_requested() && client->running.load()) {
          std::this_thread::sleep_for(std::chrono::seconds(1));
        }
      }) {
}
} // namespace net::minecraft::client::util
