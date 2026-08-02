#pragma once
#include <cstdint>
#include <string>
namespace net::minecraft::client::diagnostics {
void setStartupPhase(const char* phase);
// Records how long the phase that just ended took. Written to startup-profile.log
// next to the executable and echoed to stderr. Enabled by MINECRAFT_STARTUP_PROFILE=1.
void markStartupStep(const char* label);
// RAII form of markStartupStep for a scoped sub-step.
class StartupStep {
 public:
 explicit StartupStep(const char* label) : label_(label) {
 }
 ~StartupStep() {
  markStartupStep(label_);
 }
 StartupStep(const StartupStep&) = delete;
 StartupStep& operator=(const StartupStep&) = delete;

 private:
 const char* label_;
};
// Cumulative wall-time + call count per named work category. Safe to use from
// any thread (the shader-compile workers, IO, ...), so the coarse startup phases
// can be broken down into the work they hide. Enabled by MINECRAFT_STARTUP_PROFILE=1.
class WorkSpan {
 public:
 explicit WorkSpan(const char* category);
 ~WorkSpan();
 WorkSpan(const WorkSpan&) = delete;
 WorkSpan& operator=(const WorkSpan&) = delete;

 private:
 const char* category_;
 std::int64_t startNanos_;
};
// Records a manually measured interval into a work bucket (call count += 1).
// Pass elapsedNanos == 0 for a pure hit/miss counter.
void recordWorkSpan(const char* category, std::int64_t elapsedNanos);
// Multi-line breakdown of all recorded work buckets, sorted by total time.
// Empty string when profiling is disabled or nothing was recorded.
std::string startupWorkSummary();
// Writes startupWorkSummary() to stderr and startup-profile.log.
void dumpStartupWork();
#ifdef _WIN32
void installCrashDiagnostics();
void reportFatalError(const std::string& title, const std::string& details);
void pauseBeforeExit();
void installHangWatchdog();
void pingMainLoopHeartbeat();
void disarmHangWatchdog();
#endif
} // namespace net::minecraft::client::diagnostics
