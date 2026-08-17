#include <exception>
#include "net/minecraft/client/ClientLog.hpp"
#include "net/minecraft/client/Minecraft.hpp"
#include "net/minecraft/client/diagnostics/ClientDiagnostics.hpp"
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <timeapi.h>
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
  net::minecraft::client::diagnostics::setStartupPhase("main: starting client");
  const int exitCode = net::minecraft::client::Minecraft::main(argc, argv);
  return exitCode;
 } catch(const std::exception& exception) {
  const std::string details = std::string("Uncaught exception in main: ") + exception.what();
#ifdef _WIN32
  net::minecraft::client::diagnostics::reportFatalError("Minecraft Native - startup failed", details);
  net::minecraft::client::diagnostics::pauseBeforeExit();
#endif
  return 1;
 } catch(...) {
  const std::string details = "Uncaught unknown exception in main.";
#ifdef _WIN32
  net::minecraft::client::diagnostics::reportFatalError("Minecraft Native - startup failed", details);
  net::minecraft::client::diagnostics::pauseBeforeExit();
#endif
  return 1;
 }
}
