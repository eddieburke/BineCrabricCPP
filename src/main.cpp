#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/mod/runtime/ModBootstrap.hpp"
#include <exception>
#include <iostream>
#ifdef _WIN32
#include "net/minecraft/client/lifecycle/ClientInitializer.hpp"
#endif
int main(int argc, char** argv) {
#ifdef _WIN32
  net::minecraft::client::lifecycle::installCrashDiagnostics();
  net::minecraft::client::lifecycle::installHangWatchdog();
#endif
  try {
#ifdef _WIN32
    net::minecraft::client::lifecycle::setStartupPhase("main: starting client");
#endif
    const int exitCode = net::minecraft::client::Minecraft::main(argc, argv);
    net::minecraft::mod::runtime::shutdownClient();
    return exitCode;
  } catch(const std::exception& exception) {
    const std::string details = std::string("Uncaught exception in main: ") + exception.what();
#ifdef _WIN32
    net::minecraft::client::lifecycle::reportFatalError("Minecraft Native - startup failed", details);
    net::minecraft::client::lifecycle::pauseBeforeExit();
#endif
    std::cerr << details << std::endl;
    return 1;
  } catch(...) {
    const std::string details = "Uncaught unknown exception in main.";
#ifdef _WIN32
    net::minecraft::client::lifecycle::reportFatalError("Minecraft Native - startup failed", details);
    net::minecraft::client::lifecycle::pauseBeforeExit();
#endif
    std::cerr << details << std::endl;
    return 1;
  }
}
