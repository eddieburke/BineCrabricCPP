#include <exception>
#include <iostream>
#include "net/minecraft/client/ClientLog.hpp"
#include "net/minecraft/client/Minecraft.hpp"
#ifdef _WIN32
#include <timeapi.h>
#include <windows.h>
#include "net/minecraft/client/diagnostics/ClientDiagnostics.hpp"
struct WindowsTimerResolutionReserver {
 WindowsTimerResolutionReserver() {
  timeBeginPeriod(1);
 }
 ~WindowsTimerResolutionReserver() {
  timeEndPeriod(1);
 }
};
#endif
int main(int argc, char** argv) {
#ifdef _WIN32
 WindowsTimerResolutionReserver timerReserver;
 net::minecraft::client::diagnostics::installCrashDiagnostics();
 net::minecraft::client::diagnostics::installHangWatchdog();
#endif
 net::minecraft::client::ClientLog::init();
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
