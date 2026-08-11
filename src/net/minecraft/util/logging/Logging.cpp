#include "net/minecraft/util/logging/Logging.hpp"
#include <algorithm>
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
// Leaked singletons: the dispatcher's writer thread outlives the main thread,
// so these must never be destroyed at process exit (static destruction order
// would otherwise race the writer). The OS reclaims them on teardown.
std::mutex& loggerMutex() {
 static std::mutex* value = new std::mutex;
 return *value;
}
std::unordered_map<std::string, std::unique_ptr<Logger>>& loggers() {
 static std::unordered_map<std::string, std::unique_ptr<Logger>>* value =
     new std::unordered_map<std::string, std::unique_ptr<Logger>>;
 return *value;
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
 if(++recordsSinceFlush_ >= kFlushInterval) {
  recordsSinceFlush_ = 0;
  output_.flush();
 }
}
void FileLogHandler::flush() {
 std::lock_guard<std::mutex> lock(mutex_);
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
 if(thrown != nullptr) {
  record.message += '\n';
  record.message += ConsoleFormatter::formatThrown(*thrown);
 }
 // The exception object only lives for the duration of this call, so fold it
 // into the message now and never hand a dangling pointer to the writer.
 record.thrown = nullptr;
 LogDispatcher::instance().enqueue(this, std::move(record));
}
void Logger::publish(const LogRecord& record) {
 std::lock_guard<std::mutex> lock(mutex_);
 for(const std::unique_ptr<LogHandler>& handler : handlers_) {
  if(handler != nullptr) {
   handler->publish(record);
  }
 }
}
void Logger::flush() {
 std::lock_guard<std::mutex> lock(mutex_);
 for(const std::unique_ptr<LogHandler>& handler : handlers_) {
  if(handler != nullptr) {
   handler->flush();
  }
 }
}
LogDispatcher& LogDispatcher::instance() {
 static LogDispatcher* value = new LogDispatcher;
 return *value;
}
void LogDispatcher::start() {
 std::lock_guard<std::mutex> lock(mutex_);
 if(running_) {
  return;
 }
 running_ = true;
 thread_ = std::thread(&LogDispatcher::writerMain, this);
}
void LogDispatcher::shutdown() {
 {
  std::lock_guard<std::mutex> lock(mutex_);
  if(!running_) {
   return;
  }
  running_ = false;
 }
 cv_.notify_all();
 if(thread_.joinable()) {
  thread_.join();
 }
}
void LogDispatcher::enqueue(Logger* logger, LogRecord record) {
 const auto enqueueStart = std::chrono::steady_clock::now();
 record.logger = logger;
 std::lock_guard<std::mutex> lock(mutex_);
 ++enqueued_;
 if(queue_.size() >= kMaxQueued) {
  queue_.pop_front();
  ++dropped_;
 }
 const bool wakeWriter = queue_.empty();
 queue_.push_back(std::move(record));
 maxQueueDepth_ = std::max(maxQueueDepth_, queue_.size());
 if(wakeWriter && running_) {
  cv_.notify_one();
 }
 enqueueCpuNanos_ += static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(
                                                    std::chrono::steady_clock::now() - enqueueStart)
                                                    .count());
}
LogDispatcherStats LogDispatcher::stats() const {
 std::lock_guard<std::mutex> lock(mutex_);
 return {queue_.size(), maxQueueDepth_, enqueued_, written_, dropped_, enqueueCpuNanos_, writerCpuNanos_};
}
void LogDispatcher::writerMain() {
 std::unique_lock<std::mutex> lock(mutex_);
 for(;;) {
  cv_.wait(lock, [this] { return !queue_.empty() || !running_; });
  while(!queue_.empty()) {
   LogRecord record = std::move(queue_.front());
   queue_.pop_front();
   lock.unlock();
   const auto writeStart = std::chrono::steady_clock::now();
   record.logger->publish(record);
   const auto writeNanos = static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(
                                                          std::chrono::steady_clock::now() - writeStart)
                                                          .count());
   lock.lock();
   writerCpuNanos_ += writeNanos;
   ++written_;
  }
  if(!running_) {
   break;
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
