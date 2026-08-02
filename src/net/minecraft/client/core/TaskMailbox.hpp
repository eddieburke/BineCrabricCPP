#pragma once
#include <cstddef>
#include <deque>
#include <functional>
#include <mutex>
namespace net::minecraft::client::core {
// Main-thread drain queues (urgent / tick / render), unused until WI-12b.
// Producers push from any thread; consumers drain on the main thread only.
class TaskMailbox {
 public:
 using Task = std::function<void()>;
 void pushUrgent(Task task);
 void pushTick(Task task);
 void pushRender(Task task);
 [[nodiscard]] std::size_t drainUrgent();
 [[nodiscard]] std::size_t drainTick();
 [[nodiscard]] std::size_t drainRender();
 [[nodiscard]] std::size_t size() const;

 private:
 [[nodiscard]] std::size_t drainOne(std::deque<Task>& queue);

 mutable std::mutex mutex_;
 std::deque<Task> urgent_;
 std::deque<Task> tick_;
 std::deque<Task> render_;
};
} // namespace net::minecraft::client::core
