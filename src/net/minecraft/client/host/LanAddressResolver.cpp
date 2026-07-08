#include "net/minecraft/client/host/LanAddressResolver.hpp"

#include <algorithm>
#include <array>
#include <cstring>
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#endif
namespace net::minecraft::client::host {
namespace {
bool isPrivateIpv4(const std::string& address) {
    if (address.rfind("10.", 0) == 0 || address.rfind("192.168.", 0) == 0) {
        return true;
    }
    if (address.rfind("172.", 0) != 0) {
        return false;
    }
    const std::size_t dot = address.find('.', 4);
    if (dot == std::string::npos) {
        return false;
    }
    try {
        const int octet = std::stoi(address.substr(4, dot - 4));
        return octet >= 16 && octet <= 31;
    } catch (...) {
        return false;
    }
}

bool isUsableIpv4(const std::string& address) {
    return !address.empty() && address != "127.0.0.1" && address.rfind("169.254.", 0) != 0;
}

std::string withPort(const std::string& host, std::uint16_t port) {
    return host + ":" + std::to_string(port);
}
}  // namespace

LanConnectionInfo LanAddressResolver::resolve(std::uint16_t port) {
    LanConnectionInfo info;
    info.loopbackEndpoint = withPort("127.0.0.1", port);
    char hostName[256]{};
    if (::gethostname(hostName, static_cast<int>(sizeof(hostName))) != 0) {
        return info;
    }
    addrinfo hints{};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    addrinfo* results = nullptr;
    if (::getaddrinfo(hostName, nullptr, &hints, &results) != 0) {
        return info;
    }
    std::vector<std::string> hosts;
    for (addrinfo* current = results; current != nullptr; current = current->ai_next) {
        if (current->ai_addr == nullptr || current->ai_family != AF_INET) {
            continue;
        }
        const auto* ipv4 = reinterpret_cast<const sockaddr_in*>(current->ai_addr);
        std::array<char, INET_ADDRSTRLEN> buffer{};
        if (::inet_ntop(AF_INET, &ipv4->sin_addr, buffer.data(), static_cast<socklen_t>(buffer.size())) == nullptr) {
            continue;
        }
        const std::string address(buffer.data());
        if (!isUsableIpv4(address)) {
            continue;
        }
        if (std::find(hosts.begin(), hosts.end(), address) == hosts.end()) {
            hosts.push_back(address);
        }
    }
    ::freeaddrinfo(results);
    if (hosts.empty()) {
        return info;
    }
    auto primary = std::find_if(hosts.begin(), hosts.end(), isPrivateIpv4);
    if (primary == hosts.end()) {
        primary = hosts.begin();
    }
    info.primaryEndpoint = withPort(*primary, port);
    info.availableEndpoints.reserve(hosts.size());
    info.availableEndpoints.push_back(info.primaryEndpoint);
    for (const std::string& host : hosts) {
        const std::string endpoint = withPort(host, port);
        if (endpoint != info.primaryEndpoint) {
            info.availableEndpoints.push_back(endpoint);
        }
    }
    return info;
}
}  // namespace net::minecraft::client::host
