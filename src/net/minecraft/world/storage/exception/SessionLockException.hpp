#pragma once
#include <stdexcept>
#include <string>

namespace net::minecraft {
class SessionLockException : public std::runtime_error {
   public:
    explicit SessionLockException(const std::string& message) : std::runtime_error(message) {
    }
};
}  // namespace net::minecraft
