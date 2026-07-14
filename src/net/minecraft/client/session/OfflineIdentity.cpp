#include "net/minecraft/client/session/OfflineIdentity.hpp"
#include <mutex>
#include "net/minecraft/client/auth/microsoft/SessionRestore.hpp"
namespace net::minecraft::client::session {
namespace {
std::mutex gOfflineUsernameMutex;
std::string gOfflineUsername;
bool gHasOfflineUsername = false;
} // namespace
void setOfflineUsername(std::string name) {
  std::lock_guard lock(gOfflineUsernameMutex);
  gOfflineUsername = std::move(name);
  gHasOfflineUsername = !gOfflineUsername.empty();
}
void clearOfflineUsername() {
  std::lock_guard lock(gOfflineUsernameMutex);
  gOfflineUsername.clear();
  gHasOfflineUsername = false;
}
bool hasOfflineUsername() {
  std::lock_guard lock(gOfflineUsernameMutex);
  return gHasOfflineUsername;
}
const std::string& offlineUsername() {
  std::lock_guard lock(gOfflineUsernameMutex);
  return gOfflineUsername;
}
std::string resolveJoinUsername(const util::Session& session) {
  if(!msauth::isAuthenticated(session)) {
    std::lock_guard lock(gOfflineUsernameMutex);
    if(gHasOfflineUsername) {
      return gOfflineUsername;
    }
  }
  return session.username;
}
} // namespace net::minecraft::client::session
