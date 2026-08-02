#pragma once
#include <chrono>
#include <functional>
#include <mutex>
#include <string>
#include <utility>
#include <vector>
namespace net::minecraft::util::concurrent {
class Lifecycle {
 public:
  struct Owner {
   std::function<void()> unblock;  // wake any wait so stop() is observed
   std::function<void()> stop;     // request_stop / close blocking handles
   std::function<bool(std::chrono::steady_clock::time_point)> join;
   std::chrono::milliseconds deadline = std::chrono::seconds(3);
  };
  Lifecycle() = default;
  Lifecycle(const Lifecycle&) = delete;
  Lifecycle& operator=(const Lifecycle&) = delete;
  static Lifecycle& instance() noexcept;
  void registerOwner(std::string name, Owner owner);
  void shutdown();
  [[nodiscard]] std::size_t ownerCount() const noexcept;

 private:
  mutable std::mutex mutex_;
  std::vector<std::pair<std::string, Owner>> owners_;
  bool shutdown_ = false;
};
} // namespace net::minecraft::util::concurrent
