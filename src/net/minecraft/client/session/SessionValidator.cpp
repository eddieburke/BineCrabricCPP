#include "net/minecraft/client/session/SessionValidator.hpp"
#include <chrono>
#include <sstream>
#include <thread>
#include <utility>
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/auth/microsoft/SecretProtection.hpp"
#include "net/minecraft/client/resource/ResourceDownloadThread.hpp"
#include "net/minecraft/client/util/Session.hpp"
namespace net::minecraft::client::session {
namespace resource = net::minecraft::client::resource;
namespace {
std::int64_t currentTimeMillis() {
  return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
      .count();
}
class SessionCheckThread : public std::thread {
public:
  explicit SessionCheckThread(util::Session session)
      : std::thread([session = std::move(session)]() mutable {
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
        }) {
  }
};
} // namespace
void SessionValidator::startSessionCheck(Minecraft& client) {
  SessionCheckThread(client.session).detach();
}
} // namespace net::minecraft::client::session
