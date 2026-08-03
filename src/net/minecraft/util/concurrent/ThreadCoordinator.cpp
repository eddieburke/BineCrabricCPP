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
 std::lock_guard lock(mutex_);
 if(configured_) {
  return;
 }
 budget_ = ThreadBudget::derive(hardwareThreads, reservedThreads, options.maxComputeThreads);
 configured_ = true;
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
  // NetIo/Audio/Log are reserved-count domains: a single placeholder pool here,
  // real blocking threads tracked via reserveDynamic bookkeeping (never deducted).
  if(placeholderPool_ == nullptr) {
   placeholderPool_ = std::make_unique<WorkerPool>(1U);
  }
  return *placeholderPool_;
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
 std::unique_ptr<WorkerPool> placeholder;
 {
  std::lock_guard lock(mutex_);
  if(shutdown_) {
   return;
  }
  shutdown_ = true;
  compute = std::move(computePool_);
  io = std::move(ioPool_);
  glCompile = std::move(glCompilePool_);
  placeholder = std::move(placeholderPool_);
 }
 // Pool destructors request stop and join their workers; run outside the lock
 // so a draining task can still reach the coordinator if it needs to.
}
} // namespace net::minecraft::util::concurrent
