#pragma once
#include <chrono>
#include <exception>
#include <fstream>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>
namespace net::minecraft::server {
enum class LogLevel { Finest,
                      Finer,
                      Fine,
                      Info,
                      Warning,
                      Severe };
struct LogRecord {
  std::chrono::system_clock::time_point millis = std::chrono::system_clock::now();
  LogLevel level = LogLevel::Info;
  std::string message;
  const std::exception* thrown = nullptr;
};
class LogFormatter {
public:
  virtual ~LogFormatter() = default;
  [[nodiscard]] virtual std::string format(const LogRecord& record) const = 0;
};
class LogHandler {
public:
  virtual ~LogHandler() = default;
  void setFormatter(std::shared_ptr<LogFormatter> formatter);
  virtual void publish(const LogRecord& record) = 0;

protected:
  std::shared_ptr<LogFormatter> formatter_;
};
class ConsoleLogHandler final : public LogHandler {
public:
  void publish(const LogRecord& record) override;
};
class FileLogHandler final : public LogHandler {
public:
  FileLogHandler(const std::string& path, bool append);
  void publish(const LogRecord& record) override;

private:
  std::mutex mutex_;
  std::ofstream output_;
};
class Logger {
public:
  static Logger& getLogger(const std::string& name);
  void setUseParentHandlers(bool value);
  void addHandler(std::unique_ptr<LogHandler> handler);
  void info(const std::string& message);
  void log(LogLevel level, const std::string& message, const std::exception* thrown = nullptr);

private:
  bool useParentHandlers_ = true;
  std::vector<std::unique_ptr<LogHandler>> handlers_;
};
class ServerLog {
public:
  static Logger& LOGGER;
  static void init();
};
} // namespace net::minecraft::server
