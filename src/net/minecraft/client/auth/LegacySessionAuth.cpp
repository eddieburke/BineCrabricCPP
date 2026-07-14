#include "net/minecraft/client/auth/LegacySessionAuth.hpp"
#include <algorithm>
#include <cctype>
#include <sstream>
#include "net/minecraft/client/auth/microsoft/MicrosoftAuth.hpp"
#include "net/minecraft/client/auth/microsoft/SecretProtection.hpp"
#include "net/minecraft/client/auth/microsoft/SessionRestore.hpp"
#include "net/minecraft/client/resource/ResourceDownloadThread.hpp"
namespace net::minecraft::client::auth {
namespace resource = net::minecraft::client::resource;
namespace {
[[nodiscard]] std::string firstLine(const std::string& body) {
  const std::size_t end = body.find_first_of("\r\n");
  if(end == std::string::npos) {
    return body;
  }
  return body.substr(0, end);
}
[[nodiscard]] bool equalsIgnoreCase(const std::string& left, const std::string& right) {
  if(left.size() != right.size()) {
    return false;
  }
  for(std::size_t i = 0; i < left.size(); ++i) {
    if(std::tolower(static_cast<unsigned char>(left[i])) != std::tolower(static_cast<unsigned char>(right[i]))) {
      return false;
    }
  }
  return true;
}
[[nodiscard]] std::string urlEncodeComponent(const std::string& value) {
  static const char hex[] = "0123456789ABCDEF";
  std::string encoded;
  encoded.reserve(value.size());
  for(const unsigned char ch : value) {
    if((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9') || ch == '-' ||
       ch == '_' || ch == '.' || ch == '~') {
      encoded += static_cast<char>(ch);
    } else {
      encoded += '%';
      encoded += hex[ch >> 4];
      encoded += hex[ch & 0x0F];
    }
  }
  return encoded;
}
[[nodiscard]] JoinServerResult verifyLegacyJoinServer(const std::string& username,
                                                      const std::string& sessionId,
                                                      const std::string& serverId,
                                                      const std::atomic_bool* canceled) {
  JoinServerResult result;
  std::ostringstream url;
  url << "http://www.minecraft.net/game/joinserver.jsp"
      << "?user=" << urlEncodeComponent(username) << "&sessionId=" << urlEncodeComponent(sessionId)
      << "&serverId=" << urlEncodeComponent(serverId);
  resource::HttpRequest request;
  request.url = url.str();
  request.useBetacraftProxy = false;
  request.maxResponseBytes = 64U * 1024U;
  request.cancelled = canceled;
  const resource::HttpResponse response = resource::httpRequest(request);
  if(!response.ok()) {
    result.error = "HTTP " + std::to_string(response.statusCode);
    result.responseLine = firstLine(response.bodyAsString());
    return result;
  }
  result.responseLine = firstLine(response.bodyAsString());
  result.ok = equalsIgnoreCase(result.responseLine, "ok");
  return result;
}
[[nodiscard]] std::string modernJoinFailureText(int statusCode, const std::string& body) {
  if(const std::optional<std::string> message = msauth::json::stringField(body, "errorMessage")) {
    return *message;
  }
  if(const std::optional<std::string> error = msauth::json::stringField(body, "error")) {
    return *error;
  }
  if(!body.empty()) {
    return firstLine(body);
  }
  return "HTTP " + std::to_string(statusCode);
}
[[nodiscard]] std::string microsoftProfileId(const net::minecraft::client::util::Session& session) {
  return session.sessionId.rfind("msa:", 0) == 0 ? session.sessionId.substr(4) : std::string{};
}
[[nodiscard]] JoinServerResult verifyMicrosoftJoinServer(const net::minecraft::client::util::Session& session,
                                                         const std::string& serverId,
                                                         const std::atomic_bool* canceled) {
  JoinServerResult result;
  if(!msauth::isAuthenticated(session)) {
    result.error = "Missing Microsoft session";
    return result;
  }
  const std::string profileId = microsoftProfileId(session);
  if(profileId.empty()) {
    result.error = "Missing Microsoft profile";
    return result;
  }
  std::string body = std::string(R"({"accessToken":")") + msauth::json::escape(session.mpPass) +
                     R"(","selectedProfile":")" + msauth::json::escape(profileId) + R"(","serverId":")" +
                     msauth::json::escape(serverId) + R"("})";
  resource::HttpRequest request;
  request.method = "POST";
  request.url = "https://sessionserver.mojang.com/session/minecraft/join";
  request.headers = {
      {"Content-Type", "application/json"},
      {"Accept", "application/json"},
  };
  request.body = body;
  request.useBetacraftProxy = false;
  request.maxResponseBytes = 64U * 1024U;
  request.cancelled = canceled;
  const resource::HttpResponse response = resource::httpRequest(request);
  msauth::secret::wipeString(request.body);
  msauth::secret::wipeString(body);
  if(response.statusCode == 0) {
    result.error = "HTTP 0";
    return result;
  }
  if(response.ok()) {
    result.ok = true;
    result.responseLine = "ok";
    return result;
  }
  result.responseLine = modernJoinFailureText(response.statusCode, response.bodyAsString());
  return result;
}
} // namespace
JoinServerResult verifyJoinServer(const net::minecraft::client::util::Session& session,
                                  const std::string& serverId,
                                  const std::atomic_bool* canceled) {
  if(msauth::isAuthenticated(session)) {
    return verifyMicrosoftJoinServer(session, serverId, canceled);
  }
  return verifyLegacyJoinServer(session.username, session.sessionId, serverId, canceled);
}
} // namespace net::minecraft::client::auth
