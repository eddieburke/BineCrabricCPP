#pragma once
#include <string>
namespace net::minecraft::client {
class Minecraft;
}
namespace net::minecraft::client::lifecycle {
void setStartupPhase(const char* phase);
#ifdef _WIN32
void installCrashDiagnostics();
void reportFatalError(const std::string& title, const std::string& details);
void pauseBeforeExit();
/// Starts a background thread that watches for main-loop stalls (deadlock/infinite
/// loop) the unhandled-exception filter can't catch since no exception is thrown.
/// Call pingMainLoopHeartbeat() once per iteration of Minecraft::run()'s loop.
void installHangWatchdog();
void pingMainLoopHeartbeat();
#endif
/// Client bootstrap after Display.create(): options, textures, renderer, title screen.
/// Anchor: Minecraft.cpp L204–277.
class ClientInitializer {
public:
  static void bootstrap(Minecraft& client);
};
} // namespace net::minecraft::client::lifecycle
