#pragma once
#include <exception>
#include <string>

namespace net::minecraft::util::crash {
class CrashReport {
   public:
    std::string description;
    std::exception_ptr exception;
    CrashReport(std::string description, std::exception_ptr exception) noexcept;
    CrashReport(std::string description, std::string message);
};
}  // namespace net::minecraft::util::crash
