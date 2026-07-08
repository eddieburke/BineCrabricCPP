#include "net/minecraft/client/Minecraft.hpp"
#include <exception>
#include <iostream>
#ifdef _WIN32
#include "net/minecraft/client/diagnostics/ClientDiagnostics.hpp"
#endif
int main(int argc, char** argv) {
#ifdef _WIN32
  net::minecraft::client::diagnostics::installCrashDiagnostics();
  net::minecraft::client::diagnostics::installHangWatchdog();
#endif
  try {
#ifdef _WIN32
    net::minecraft::client::diagnostics::setStartupPhase("main: starting client");
#endif
    const int exitCode = net::minecraft::client::Minecraft::main(argc, argv);
    return exitCode;
  } catch(const std::exception& exception) {
    const std::string details = std::string("Uncaught exception in main: ") + exception.what();
#ifdef _WIN32
    net::minecraft::client::diagnostics::reportFatalError("Minecraft Native - startup failed", details);
    net::minecraft::client::diagnostics::pauseBeforeExit();
#endif
    std::cerr << details << std::endl;
    return 1;
  } catch(...) {
    const std::string details = "Uncaught unknown exception in main.";
#ifdef _WIN32
    net::minecraft::client::diagnostics::reportFatalError("Minecraft Native - startup failed", details);
    net::minecraft::client::diagnostics::pauseBeforeExit();
#endif
    std::cerr << details << std::endl;
    return 1;
  }
}
