#include "net/minecraft/util/concurrent/ThreadCoordinator.hpp"
namespace net::minecraft::util::concurrent {
ThreadCoordinator::ThreadCoordinator() noexcept : budget_(ThreadBudget::derive(std::thread::hardware_concurrency())) {
}
ThreadCoordinator& ThreadCoordinator::instance() noexcept {
 static ThreadCoordinator coordinator;
 return coordinator;
}
void ThreadCoordinator::configure(unsigned hardwareThreads, unsigned reservedThreads) {
 configure(hardwareThreads, reservedThreads, Options{});
}
void ThreadCoordinator::configure(unsigned hardwareThreads, unsigned reservedThreads, Options options) {
 std::unique_ptr<WorkerPool> oldComputePool;
 std::unique_ptr<WorkerPool> oldIoPool;
 std::unique_ptr<WorkerPool> oldGlCompilePool;
 {
  std::lock_guard lock(mutex_);
  if(configured_) {
   return;
  }
  budget_ = ThreadBudget::derive(hardwareThreads, reservedThreads, options.maxComputeThreads);
  if(computePool_) {
   oldComputePool = std::move(computePool_);
   computePool_ = std::make_unique<WorkerPool>(budget_.compute);
  }
  if(ioPool_) {
   oldIoPool = std::move(ioPool_);
   ioPool_ = std::make_unique<WorkerPool>(budget_.io);
  }
  if(glCompilePool_) {
   oldGlCompilePool = std::move(glCompilePool_);
   glCompilePool_ = std::make_unique<WorkerPool>(budget_.glCompile);
  }
  configured_ = true;
 }
}
WorkerPool& ThreadCoordinator::pool(Domain domain) {
 std::lock_guard lock(mutex_);
 switch(domain) {
 case Domain::Compute:
  if(computePool_ == nullptr) {
   computePool_ = std::make_unique<WorkerPool>(budget_.compute);
  }
  return *computePool_;
 case Domain::Io:
  if(ioPool_ == nullptr) {
   ioPool_ = std::make_unique<WorkerPool>(budget_.io);
  }
  return *ioPool_;
 case Domain::GlCompile:
  if(glCompilePool_ == nullptr) {
   glCompilePool_ = std::make_unique<WorkerPool>(budget_.glCompile);
  }
  return *glCompilePool_;
 default:
  // NetIo/Audio/Log are tag-only domains: their blocking threads are started by
  // the owners (Connection, server listeners) and never go through a pool.
  std::abort();
 }
}
ThreadBudget ThreadCoordinator::budget() const noexcept {
 std::lock_guard lock(mutex_);
 return budget_;
}
std::uint64_t ThreadCoordinator::totalPending() const noexcept {
 std::lock_guard lock(mutex_);
 std::uint64_t total = 0;
 if(computePool_ != nullptr) {
  total += computePool_->pendingCount();
 }
 if(ioPool_ != nullptr) {
  total += ioPool_->pendingCount();
 }
 if(glCompilePool_ != nullptr) {
  total += glCompilePool_->pendingCount();
 }
 return total;
}
unsigned ThreadCoordinator::computeShare(unsigned owners) const noexcept {
 std::lock_guard lock(mutex_);
 return budget_.computeShare(owners);
}
void ThreadCoordinator::shutdown() {
 std::unique_ptr<WorkerPool> compute;
 std::unique_ptr<WorkerPool> io;
 std::unique_ptr<WorkerPool> glCompile;
 {
  std::lock_guard lock(mutex_);
  if(shutdown_) {
   return;
  }
  shutdown_ = true;
  compute = std::move(computePool_);
  io = std::move(ioPool_);
  glCompile = std::move(glCompilePool_);
 }
 // Pool destructors request stop and join their workers; run outside the lock
 // so a draining task can still reach the coordinator if it needs to.
}
} // namespace net::minecraft::util::concurrent
