#pragma once

#include <string>

namespace net::minecraft {
class MinecraftApplet;
}

namespace net::minecraft::client {
class Minecraft;
}

namespace net::minecraft::client::lifecycle {

/// Entry points: main(), start(), RunnableMinecraft crash handler.
/// Anchor: Minecraft.cpp L103–120, L1353–1407.
class ClientLaunch {
public:
    static void start(const std::string& username, const std::string& sessionId);
    static void startAndConnect(const std::string& username, const std::string& sessionId, const std::string* server);
    static int main(int argc, char** argv);
};

} // namespace net::minecraft::client::lifecycle
