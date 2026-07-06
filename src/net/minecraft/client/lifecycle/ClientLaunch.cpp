#include "net/minecraft/client/lifecycle/ClientLaunch.hpp"
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/mod/runtime/ModBootstrap.hpp"
#include "net/minecraft/client/CrashReportPanel.hpp"
#include "net/minecraft/client/util/Session.hpp"
#include "net/minecraft/util/crash/CrashReport.hpp"
#include <chrono>
#include <iostream>
#include <memory>
#ifdef _WIN32
#include "net/minecraft/client/lifecycle/ClientInitializer.hpp"
#endif
namespace net::minecraft::client::lifecycle {
namespace {
std::int64_t currentTimeMillis() {
  return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
      .count();
}
class RunnableMinecraft final : public Minecraft {
public:
  RunnableMinecraft(void* component, void* canvas, net::minecraft::MinecraftApplet* applet, int width, int height,
                    bool fullscreen)
      : Minecraft(component, canvas, applet, width, height, fullscreen) {
  }
  void handleCrash(const net::minecraft::util::crash::CrashReport& crashReport) override {
    const CrashReportPanel panel(crashReport);
    const std::string& reportText = panel.reportText();
    std::cerr << reportText << std::endl;
#ifdef _WIN32
    net::minecraft::client::lifecycle::reportFatalError("Minecraft has crashed!", reportText);
    net::minecraft::client::lifecycle::pauseBeforeExit();
#endif
  }
};
} // namespace
void ClientLaunch::start(const std::string& username, const std::string& sessionId) {
  startAndConnect(username, sessionId, nullptr);
}
void ClientLaunch::startAndConnect(const std::string& username, const std::string& sessionId, const std::string* server) {
  auto client = std::make_unique<RunnableMinecraft>(nullptr, nullptr, nullptr, 854, 480, false);
  client->hostAddress = "www.minecraft.net";
  if(!username.empty() && !sessionId.empty()) {
    client->session = util::Session(username, sessionId);
  } else {
    client->session = util::Session("Player" + std::to_string(currentTimeMillis() % 1000LL), "");
  }
  if(server != nullptr) {
    const std::size_t colon = server->find(':');
    if(colon != std::string::npos) {
      client->setStartupServer(server->substr(0, colon), std::stoi(server->substr(colon + 1)));
    }
  }
  client->run();
}
int ClientLaunch::main(int argc, char** argv) {
  net::minecraft::mod::runtime::bootstrapClient();
  std::string username = "Player" + std::to_string(currentTimeMillis() % 1000LL);
  std::string sessionId = "-";
  if(argc > 1 && argv[1] != nullptr) {
    username = argv[1];
  }
  if(argc > 2 && argv[2] != nullptr) {
    sessionId = argv[2];
  }
  try {
    start(username, sessionId);
  } catch(const std::exception& exception) {
    std::cerr << exception.what() << std::endl;
#ifdef _WIN32
    net::minecraft::client::lifecycle::reportFatalError("Minecraft Native - failed to start",
                                                        std::string(exception.what()));
    net::minecraft::client::lifecycle::pauseBeforeExit();
#endif
    return 1;
  } catch(...) {
    const char* message = "Unknown exception during startup.";
    std::cerr << message << std::endl;
#ifdef _WIN32
    net::minecraft::client::lifecycle::reportFatalError("Minecraft Native - failed to start", message);
    net::minecraft::client::lifecycle::pauseBeforeExit();
#endif
    return 1;
  }
  return 0;
}
} // namespace net::minecraft::client::lifecycle
