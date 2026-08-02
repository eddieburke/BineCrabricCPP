#pragma once
#include <cstdint>
#include <memory>
#include <mutex>
#include <thread>
#include <utility>
#include "net/minecraft/util/concurrent/ThreadBudget.hpp"
#include "net/minecraft/util/concurrent/WorkerPool.hpp"
namespace net::minecraft::util::concurrent {
enum class Domain { Compute, Io, GlCompile, NetIo, Audio, Log };
enum class TaskPriority { Urgent = 0, High = 1, Normal = 2, Low = 3, Idle = 4 };
// Single authority for thread counts and domain pools. One budget computed once
// (Minecraft::init); Compute/Io/GlCompile get real WorkerPools, NetIo/Audio/Log
// are registered-but-not-deducted reserved counts (blocking/device threads, Q7).
class ThreadCoordinator {
 public:
  struct Options {
   unsigned maxComputeThreads = 8;
  };
  static ThreadCoordinator& instance() noexcept;
  // First configure wins; repeat calls are no-ops (idempotent). Overload avoids
  // GCC's default-argument brace-init bug for an aggregate with an NSDMI nested
  // in the same class.
  void configure(unsigned hardwareThreads, unsigned reservedThreads);
  void configure(unsigned hardwareThreads, unsigned reservedThreads, Options options);
  WorkerPool& pool(Domain domain);
  [[nodiscard]] ThreadBudget budget() const noexcept;
  void reserveDynamic(unsigned count);
  void releaseDynamic(unsigned count);
  [[nodiscard]] std::uint64_t totalPending() const noexcept;
  void shutdown();
  [[nodiscard]] unsigned computeShare(unsigned owners) const noexcept;

 private:
  ThreadCoordinator() noexcept;
  ThreadCoordinator(const ThreadCoordinator&) = delete;
  ThreadCoordinator& operator=(const ThreadCoordinator&) = delete;
  mutable std::mutex mutex_;
  ThreadBudget budget_;
  bool configured_ = false;
  bool shutdown_ = false;
  std::uint64_t dynamicReserved_ = 0;
  std::unique_ptr<WorkerPool> computePool_;
  std::unique_ptr<WorkerPool> ioPool_;
  std::unique_ptr<WorkerPool> glCompilePool_;
  std::unique_ptr<WorkerPool> placeholderPool_;
};
} // namespace net::minecraft::util::concurrent
