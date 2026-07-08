#include "net/minecraft/util/crash/CrashReport.hpp"

#include <stdexcept>
#include <utility>

namespace net::minecraft::util::crash {
CrashReport::CrashReport(std::string description, std::exception_ptr exception) noexcept
    : description(std::move(description)), exception(std::move(exception)) {
}

CrashReport::CrashReport(std::string description, std::string message)
    : description(std::move(description)), exception(std::make_exception_ptr(std::runtime_error(std::move(message)))) {
}
}  // namespace net::minecraft::util::crash
