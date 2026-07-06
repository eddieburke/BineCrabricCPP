#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#endif
#include "TestAssert.hpp"
#include "net/minecraft/server/MinecraftServer.hpp"
#include "net/minecraft/server/network/ConnectionListener.hpp"
#include <chrono>
#include <mutex>
#include <sstream>
#include <thread>
namespace {
using net::minecraft::server::MinecraftServer;
using net::minecraft::server::network::ConnectionListener;
void ensureWinsock() {
  static std::once_flag once;
  std::call_once(once, []() {
    WSADATA data{};
    if(::WSAStartup(MAKEWORD(2, 2), &data) != 0) {
      throw std::runtime_error("WSAStartup failed");
    }
  });
}
SOCKET connectLoopback(std::uint16_t port) {
  ensureWinsock();
  addrinfo hints{};
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;
  addrinfo* result = nullptr;
  std::ostringstream portStream;
  portStream << port;
  if(::getaddrinfo("127.0.0.1", portStream.str().c_str(), &hints, &result) != 0 || result == nullptr) {
    return INVALID_SOCKET;
  }
  SOCKET socket = INVALID_SOCKET;
  for(addrinfo* candidate = result; candidate != nullptr; candidate = candidate->ai_next) {
    socket = ::socket(candidate->ai_family, candidate->ai_socktype, candidate->ai_protocol);
    if(socket == INVALID_SOCKET) {
      continue;
    }
    if(::connect(socket, candidate->ai_addr, static_cast<int>(candidate->ai_addrlen)) == 0) {
      break;
    }
    ::closesocket(socket);
    socket = INVALID_SOCKET;
  }
  ::freeaddrinfo(result);
  return socket;
}
void testBindAndAcceptSmoke(TestContext& ctx) {
  MinecraftServer server;
  ConnectionListener listener(&server, "127.0.0.1", 0, false);
  const std::uint16_t port = listener.boundPort();
  EXPECT_TRUE(ctx, port != 0, "connection listener should bind to an ephemeral port");
  const SOCKET client = connectLoopback(port);
  EXPECT_TRUE(ctx, client != INVALID_SOCKET, "loopback client should connect to listener");
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  listener.tick();
  listener.close();
  if(client != INVALID_SOCKET) {
    ::shutdown(client, SD_BOTH);
    ::closesocket(client);
  }
}
} // namespace
int main() {
  TestContext ctx;
  RUN_TEST(ctx, testBindAndAcceptSmoke);
  if(ctx.failures == 0) {
    std::cout << "All connection listener tests passed\n";
    return EXIT_SUCCESS;
  }
  std::cerr << ctx.failures << " test failure(s)\n";
  return EXIT_FAILURE;
}
