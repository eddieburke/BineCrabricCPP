#include "net/minecraft/util/logging/Logging.hpp"
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <typeinfo>
#if defined(__GNUC__)
#include <cxxabi.h>
#endif
namespace net::minecraft::util::logging {
namespace {
std::mutex& loggerMutex() {
 static std::mutex value;
 return value;
}
std::unordered_map<std::string, std::unique_ptr<Logger>>& loggers() {
 static std::unordered_map<std::string, std::unique_ptr<Logger>> value;
 return value;
}
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
void LogHandler::setFormatter(std::shared_ptr<LogFormatter> formatter) {
 formatter_ = std::move(formatter);
}
void ConsoleLogHandler::publish(const LogRecord& record) {
 if(formatter_ == nullptr) {
  return;
 }
 std::cerr << formatter_->format(record);
}
FileLogHandler::FileLogHandler(const std::string& path, bool append) {
 output_.open(path, append ? std::ios::app : std::ios::trunc);
 if(!output_) {
  throw std::runtime_error("Failed to open log file: " + path);
 }
}
void FileLogHandler::publish(const LogRecord& record) {
 if(formatter_ == nullptr) {
  return;
 }
 const std::string formatted = formatter_->format(record);
 std::lock_guard<std::mutex> lock(mutex_);
 output_ << formatted;
 output_.flush();
}
Logger& Logger::getLogger(const std::string& name) {
 std::lock_guard<std::mutex> lock(loggerMutex());
 auto& instances = loggers();
 const auto existing = instances.find(name);
 if(existing != instances.end()) {
  return *existing->second;
 }
 auto logger = std::make_unique<Logger>();
 Logger& reference = *logger;
 instances.emplace(name, std::move(logger));
 return reference;
}
void Logger::setUseParentHandlers(bool value) {
 useParentHandlers_ = value;
}
void Logger::addHandler(std::unique_ptr<LogHandler> handler) {
 std::lock_guard<std::mutex> lock(mutex_);
 handlers_.push_back(std::move(handler));
}
void Logger::info(const std::string& message) {
 log(LogLevel::Info, message, nullptr);
}
void Logger::log(LogLevel level, const std::string& message, const std::exception* thrown) {
 LogRecord record;
 record.level = level;
 record.message = message;
 record.thrown = thrown;
 std::lock_guard<std::mutex> lock(mutex_);
 for(const std::unique_ptr<LogHandler>& handler : handlers_) {
  if(handler != nullptr) {
   handler->publish(record);
  }
 }
}
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
} // namespace net::minecraft::util::logging
