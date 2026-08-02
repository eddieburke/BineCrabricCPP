#include "net/minecraft/client/core/TaskMailbox.hpp"
#include <utility>
namespace net::minecraft::client::core {
void TaskMailbox::pushUrgent(Task task) {
 const std::lock_guard lock(mutex_);
 urgent_.push_back(std::move(task));
}
void TaskMailbox::pushTick(Task task) {
 const std::lock_guard lock(mutex_);
 tick_.push_back(std::move(task));
}
void TaskMailbox::pushRender(Task task) {
 const std::lock_guard lock(mutex_);
 render_.push_back(std::move(task));
}
std::size_t TaskMailbox::drainUrgent() {
 return drainOne(urgent_);
}
std::size_t TaskMailbox::drainTick() {
 return drainOne(tick_);
}
std::size_t TaskMailbox::drainRender() {
 return drainOne(render_);
}
std::size_t TaskMailbox::size() const {
 const std::lock_guard lock(mutex_);
 return urgent_.size() + tick_.size() + render_.size();
}
std::size_t TaskMailbox::drainOne(std::deque<Task>& queue) {
 std::deque<Task> local;
 {
  const std::lock_guard lock(mutex_);
  local.swap(queue);
 }
 for(const Task& task : local) {
  task();
 }
 return local.size();
}
} // namespace net::minecraft::client::core
