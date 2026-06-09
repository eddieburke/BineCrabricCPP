#include "net/minecraft/client/session/SessionValidator.hpp"

#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/resource/ResourceDownloadThread.hpp"

#include <chrono>
#include <sstream>
#include <thread>

namespace net::minecraft::client::session {

namespace resource = net::minecraft::client::resource;

namespace {

std::int64_t currentTimeMillis()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

class SessionCheckThread : public std::thread {
public:
    explicit SessionCheckThread(Minecraft* client)
        : std::thread([client]() {
            if (client == nullptr) {
                return;
            }
            std::ostringstream url;
            url << "https://login.minecraft.net/session?name=" << client->session.username
                << "&session=" << client->session.sessionId;
            const resource::HttpResponse response = resource::fetchUrl(url.str(), true);
            if (response.statusCode == 400) {
                SessionValidator::failedSessionCheckTime.store(currentTimeMillis(), std::memory_order_relaxed);
            }
        })
    {
    }
};

} // namespace

void SessionValidator::startSessionCheck(Minecraft& client)
{
    SessionCheckThread(&client).detach();
}

} // namespace net::minecraft::client::session
