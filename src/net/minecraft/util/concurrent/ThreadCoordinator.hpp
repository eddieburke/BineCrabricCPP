#pragma once
#include <cstdint>
#include <memory>
#include <mutex>
#include <thread>
#include <utility>
#include "net/minecraft/util/concurrent/ThreadBudget.hpp"
#include "net/minecraft/util/concurrent/WorkerPool.hpp"
namespace net::minecraft::util::concurrent {
enum class Domain { Compute,
                    Io,
                    GlCompile,
                    NetIo,
                    Audio,
                    Log };
class ThreadCoordinator {
 public:
 struct Options {
  unsigned maxComputeThreads = 8;
 };
 static ThreadCoordinator& instance() noexcept;
 void configure(unsigned hardwareThreads, unsigned reservedThreads);
 void configure(unsigned hardwareThreads, unsigned reservedThreads, Options options);
 WorkerPool& pool(Domain domain);
 [[nodiscard]] ThreadBudget budget() const noexcept;
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
 std::unique_ptr<WorkerPool> computePool_;
 std::unique_ptr<WorkerPool> ioPool_;
 std::unique_ptr<WorkerPool> glCompilePool_;
};
} // namespace net::minecraft::util::concurrent
