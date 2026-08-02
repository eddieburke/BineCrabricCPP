#include "net/minecraft/client/session/SessionValidator.hpp"
#include <chrono>
#include <sstream>
#include <utility>
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/auth/microsoft/SecretProtection.hpp"
#include "net/minecraft/client/resource/ResourceDownloadThread.hpp"
#include "net/minecraft/client/util/Session.hpp"
#include "net/minecraft/util/concurrent/ThreadCoordinator.hpp"
namespace net::minecraft::client::session {
namespace resource = net::minecraft::client::resource;
namespace {
std::int64_t currentTimeMillis() {
 return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
     .count();
}
void validateSession(util::Session session) {
        resource::HttpResponse response;
        if(session.sessionId.rfind("msa:", 0) == 0 && !session.mpPass.empty()) {
         resource::HttpRequest request;
         request.method = "GET";
         request.url = "https://api.minecraftservices.com/minecraft/profile";
         request.headers = {
             {"Authorization", "Bearer " + session.mpPass},
             {"Accept", "application/json"},
         };
         request.useBetacraftProxy = false;
         response = resource::httpRequest(request);
         msauth::secret::wipeString(request.headers.front().value);
         if(response.statusCode == 401 || response.statusCode == 403) {
          SessionValidator::failedSessionCheckTime.store(currentTimeMillis(), std::memory_order_relaxed);
         }
         msauth::secret::wipeString(session.mpPass);
         return;
        }
        std::ostringstream url;
        url << "https://login.minecraft.net/session?name=" << session.username
            << "&session=" << session.sessionId;
        response = resource::fetchUrl(url.str(), true);
        if(response.statusCode == 400) {
         SessionValidator::failedSessionCheckTime.store(currentTimeMillis(), std::memory_order_relaxed);
        }
}
} // namespace
void SessionValidator::startSessionCheck(Minecraft& client) {
 util::Session session = client.session;
 net::minecraft::util::concurrent::ThreadCoordinator::instance()
     .pool(net::minecraft::util::concurrent::Domain::Io)
     .submit([session = std::move(session)]() mutable { validateSession(std::move(session)); });
}
} // namespace net::minecraft::client::session
