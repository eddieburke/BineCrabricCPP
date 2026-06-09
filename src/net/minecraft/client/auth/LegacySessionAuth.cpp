#include "net/minecraft/client/auth/LegacySessionAuth.hpp"

#include "net/minecraft/client/resource/ResourceDownloadThread.hpp"

#include <algorithm>
#include <cctype>
#include <sstream>

namespace net::minecraft::client::auth {

namespace resource = net::minecraft::client::resource;

namespace {

[[nodiscard]] std::string firstLine(const std::string& body)
{
    const std::size_t end = body.find_first_of("\r\n");
    if (end == std::string::npos) {
        return body;
    }
    return body.substr(0, end);
}

[[nodiscard]] bool equalsIgnoreCase(const std::string& left, const std::string& right)
{
    if (left.size() != right.size()) {
        return false;
    }
    for (std::size_t i = 0; i < left.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(left[i])) !=
            std::tolower(static_cast<unsigned char>(right[i]))) {
            return false;
        }
    }
    return true;
}

} // namespace

JoinServerResult verifyJoinServer(
    const std::string& username,
    const std::string& sessionId,
    const std::string& serverId)
{
    JoinServerResult result;
    std::ostringstream url;
    url << "http://www.minecraft.net/game/joinserver.jsp"
        << "?user=" << username
        << "&sessionId=" << sessionId
        << "&serverId=" << serverId;

    const resource::HttpResponse response = resource::fetchUrl(url.str(), true);
    if (!response.ok()) {
        result.error = "HTTP " + std::to_string(response.statusCode);
        result.responseLine = firstLine(response.bodyAsString());
        return result;
    }

    result.responseLine = firstLine(response.bodyAsString());
    result.ok = equalsIgnoreCase(result.responseLine, "ok");
    return result;
}

} // namespace net::minecraft::client::auth
