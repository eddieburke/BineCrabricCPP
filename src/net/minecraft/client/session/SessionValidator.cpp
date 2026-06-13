#include "net/minecraft/client/session/SessionValidator.hpp"

#include "msauth/SecretProtection.hpp"
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

            resource::HttpResponse response;
            if (client->session.sessionId.rfind("msa:", 0) == 0 && !client->session.mpPass.empty()) {
                resource::HttpRequest request;
                request.method = "GET";
                request.url = "https://api.minecraftservices.com/minecraft/profile";
                request.headers = {
                    {"Authorization", "Bearer " + client->session.mpPass},
                    {"Accept", "application/json"},
                };
                request.useBetacraftProxy = false;
                response = resource::httpRequest(request);
                msauth::secret::wipeString(request.headers.front().value);
                if (response.statusCode == 401 || response.statusCode == 403) {
                    SessionValidator::failedSessionCheckTime.store(currentTimeMillis(), std::memory_order_relaxed);
                }
                return;
            }

            std::ostringstream url;
            url << "https://login.minecraft.net/session?name=" << client->session.username
                << "&session=" << client->session.sessionId;
            response = resource::fetchUrl(url.str(), true);
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
