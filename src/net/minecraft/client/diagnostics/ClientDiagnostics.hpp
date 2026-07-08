#pragma once
#include <string>

namespace net::minecraft::client::diagnostics {
void setStartupPhase(const char* phase);
#ifdef _WIN32
void installCrashDiagnostics();
void reportFatalError(const std::string& title, const std::string& details);
void pauseBeforeExit();
void installHangWatchdog();
void pingMainLoopHeartbeat();
void disarmHangWatchdog();
#endif
}  // namespace net::minecraft::client::diagnostics
