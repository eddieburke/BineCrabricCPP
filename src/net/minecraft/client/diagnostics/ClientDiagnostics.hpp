#pragma once
#include <string>
#include "net/minecraft/util/diagnostics/Instrumentation.hpp"
namespace net::minecraft::client::diagnostics {
void setStartupPhase(const char* phase);
#ifdef _WIN32
void installCrashDiagnostics();
void reportFatalError(const std::string& title, const std::string& details);
void pauseBeforeExit();
#if MINECRAFT_ENABLE_PROFILING
void installHangWatchdog();
void pingMainLoopHeartbeat();
void disarmHangWatchdog();
#else
inline void installHangWatchdog() {}
inline void pingMainLoopHeartbeat() {}
inline void disarmHangWatchdog() {}
#endif
#endif
} // namespace net::minecraft::client::diagnostics
