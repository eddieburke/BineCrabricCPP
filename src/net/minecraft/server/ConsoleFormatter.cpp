#include "net/minecraft/server/ConsoleFormatter.hpp"
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <sstream>
#include <typeinfo>
#if defined(__GNUC__)
#include <cxxabi.h>
#endif
namespace net::minecraft::server {
namespace {
constexpr const char* kLevelFinest = " [FINEST] ";
constexpr const char* kLevelFiner = " [FINER] ";
constexpr const char* kLevelFine = " [FINE] ";
constexpr const char* kLevelInfo = " [INFO] ";
constexpr const char* kLevelWarning = " [WARNING] ";
constexpr const char* kLevelSevere = " [SEVERE] ";
[[nodiscard]] std::string exceptionTypeName(const std::exception& thrown) {
#if defined(__GNUC__)
  int status = 0;
  char* demangled = abi::__cxa_demangle(typeid(thrown).name(), nullptr, nullptr, &status);
  const std::string name = (status == 0 && demangled != nullptr) ? demangled : typeid(thrown).name();
  std::free(demangled);
  return name;
#else
  return typeid(thrown).name();
#endif
}
} // namespace
std::string ConsoleFormatter::formatTimestamp(const LogRecord& record) {
  const std::time_t seconds = std::chrono::system_clock::to_time_t(record.millis);
  std::tm localTime{};
#if defined(_WIN32)
  localtime_s(&localTime, &seconds);
#else
  localtime_r(&seconds, &localTime);
#endif
  char buffer[32] = {};
  std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &localTime);
  return buffer;
}
std::string ConsoleFormatter::formatLevel(LogLevel level) {
  switch(level) {
  case LogLevel::Finest:
    return kLevelFinest;
  case LogLevel::Finer:
    return kLevelFiner;
  case LogLevel::Fine:
    return kLevelFine;
  case LogLevel::Info:
    return kLevelInfo;
  case LogLevel::Warning:
    return kLevelWarning;
  case LogLevel::Severe:
    return kLevelSevere;
  }
  return kLevelInfo;
}
std::string ConsoleFormatter::formatThrown(const std::exception& thrown) {
  std::ostringstream stackTrace;
  stackTrace << exceptionTypeName(thrown) << ": " << thrown.what() << '\n';
  return stackTrace.str();
}
std::string ConsoleFormatter::format(const LogRecord& record) const {
  std::ostringstream builder;
  builder << formatTimestamp(record);
  builder << formatLevel(record.level);
  builder << record.message;
  builder << '\n';
  if(record.thrown != nullptr) {
    builder << formatThrown(*record.thrown);
  }
  return builder.str();
}
} // namespace net::minecraft::server
