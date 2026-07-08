#include "net/minecraft/server/MinecraftServer.hpp"
#include "net/minecraft/server/ServerLog.hpp"
#include "net/minecraft/stat/Stats.hpp"
#ifdef _WIN32
#include "net/minecraft/server/dedicated/gui/DedicatedServerGui.hpp"
#endif
#include <exception>
#include <string>
#include <thread>

namespace {
[[nodiscard]] bool hasNoGuiArg(int argc, char** argv) {
    for (int i = 1; i < argc; ++i) {
        if (argv[i] != nullptr && std::string(argv[i]) == "nogui") {
            return true;
        }
    }
    return false;
}
}  // namespace

int main(int argc, char** argv) {
    using net::minecraft::server::LogLevel;
    using net::minecraft::server::MinecraftServer;
    using net::minecraft::server::ServerLog;
    using net::minecraft::stat::Stats;
    try {
        Stats::initialize();
        MinecraftServer server;
#ifdef _WIN32
        if (!hasNoGuiArg(argc, argv)) {
            net::minecraft::server::dedicated::gui::DedicatedServerGui::create(server);
        }
#endif
        std::thread serverThread([&server]() { server.run(); });
        serverThread.join();
    } catch (const std::exception& exception) {
        ServerLog::LOGGER.log(LogLevel::Severe, "Failed to start the minecraft server", &exception);
        return 1;
    }
    return 0;
}
