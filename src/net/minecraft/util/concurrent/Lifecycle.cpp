#include "net/minecraft/util/concurrent/Lifecycle.hpp"
#include <cstdio>
namespace net::minecraft::util::concurrent {
Lifecycle& Lifecycle::instance() noexcept {
 static Lifecycle lifecycle;
 return lifecycle;
}
void Lifecycle::registerOwner(std::string name, Owner owner) {
 std::lock_guard lock(mutex_);
 owners_.emplace_back(std::move(name), std::move(owner));
}
void Lifecycle::shutdown() {
 std::vector<std::pair<std::string, Owner>> owners;
 {
  std::lock_guard lock(mutex_);
  if(shutdown_) {
   return;
  }
  shutdown_ = true;
  owners = std::move(owners_);
 }
 // Unblock every owner so a blocked wait can observe the stop request.
 for(auto& [name, owner] : owners) {
  (void)name;
  if(owner.unblock) {
   try {
    owner.unblock();
   } catch(...) {
   }
  }
 }
 // Then request stop on everything.
 for(auto& [name, owner] : owners) {
  (void)name;
  if(owner.stop) {
   try {
    owner.stop();
   } catch(...) {
   }
  }
 }
 for(auto& [name, owner] : owners) {
  if(!owner.join) {
   continue;
  }
  const std::chrono::milliseconds deadline =
      owner.deadline > std::chrono::milliseconds(0) ? owner.deadline : std::chrono::seconds(3);
  bool joined = false;
  try {
   joined = owner.join(std::chrono::steady_clock::now() + deadline);
  } catch(...) {
  }
  if(!joined) {
   std::fprintf(stderr, "[Lifecycle] owner '%s' missed %lld ms deadline\n", name.c_str(),
                static_cast<long long>(deadline.count()));
  }
 }
}
std::size_t Lifecycle::ownerCount() const noexcept {
 std::lock_guard lock(mutex_);
 return owners_.size();
}
} // namespace net::minecraft::util::concurrent
