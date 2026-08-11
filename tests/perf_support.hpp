#pragma once
#include <atomic>
#include <chrono>
#include <cstdint>
#ifdef _WIN32
#include <windows.h>
#endif
namespace net::minecraft::test {
// Wall time alone cannot tell "this took 5 ms of work" from "this waited 5 ms
// for a lock". Cycles retired by *this thread* do not advance while it is
// blocked on a mutex, a fence, or the graphics driver, so wall minus busy
// isolates the stall. That is the property a contention test has to assert;
// gdb sampling shows the same thing from outside, this makes it assertable
// from inside a gtest case.
inline std::uint64_t threadCycles() {
#ifdef _WIN32
 ULONG64 cycles = 0;
 if(QueryThreadCycleTime(GetCurrentThread(), &cycles) != 0) {
  return static_cast<std::uint64_t>(cycles);
 }
#endif
 return 0;
}
inline std::int64_t nanoTime() {
 return std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now().time_since_epoch())
     .count();
}
// Cycles are not nanoseconds and the ratio is neither the nominal clock nor
// stable across machines, so measure it against a loop that by construction
// never blocks.
inline double cyclesPerNs() {
 static const double ratio = [] {
  const std::int64_t startNs = nanoTime();
  const std::uint64_t startCycles = threadCycles();
  if(startCycles == 0) {
   return 0.0;
  }
  volatile double sink = 0.0;
  std::int64_t elapsedNs = 0;
  while((elapsedNs = nanoTime() - startNs) < 2000000) {
   sink = sink + 1.0;
  }
  const std::uint64_t elapsedCycles = threadCycles() - startCycles;
  if(elapsedNs <= 0 || elapsedCycles == 0) {
   return 0.0;
  }
  return static_cast<double>(elapsedCycles) / static_cast<double>(elapsedNs);
 }();
 return ratio;
}
struct StallProfile {
 double wallMs = 0.0;
 double busyMs = 0.0;
 double blockedMs = 0.0;
 [[nodiscard]] bool supported() const {
  return cyclesPerNs() > 0.0;
 }
 [[nodiscard]] double blockedShare() const {
  return wallMs > 0.0 ? blockedMs / wallMs : 0.0;
 }
};
template <typename Fn>
StallProfile measureStall(Fn&& body) {
 const double ratio = cyclesPerNs();
 const std::int64_t startNs = nanoTime();
 const std::uint64_t startCycles = threadCycles();
 body();
 const std::uint64_t endCycles = threadCycles();
 const std::int64_t endNs = nanoTime();
 StallProfile profile;
 profile.wallMs = static_cast<double>(endNs - startNs) / 1.0e6;
 if(ratio > 0.0 && endCycles >= startCycles) {
  profile.busyMs = (static_cast<double>(endCycles - startCycles) / ratio) / 1.0e6;
  profile.blockedMs = profile.wallMs - profile.busyMs;
  if(profile.blockedMs < 0.0) {
   profile.blockedMs = 0.0;
  }
 }
 return profile;
}
// Releases every participant at once. Without this a "contended lock" test is a
// lie: std::thread spawn latency is tens of microseconds, so a holder that
// sleeps 100 us has already released the lock before the contender's thread
// runs its first instruction, and the test measures an uncontended acquire.
class StartGate {
 public:
 void arrive() {
  ++arrived_;
  while(!open_.load(std::memory_order_acquire)) {
  }
 }
 void waitFor(int participants) {
  while(arrived_.load(std::memory_order_acquire) < participants) {
  }
 }
 void open() {
  open_.store(true, std::memory_order_release);
 }

 private:
 std::atomic<int> arrived_{0};
 std::atomic<bool> open_{false};
};
} // namespace net::minecraft::test
