#include "net/minecraft/server/ServerLog.hpp"
#include "net/minecraft/server/ConsoleFormatter.hpp"
#include <iostream>
#include <mutex>
#include <stdexcept>
namespace net::minecraft::server {
namespace {
std::mutex g_loggerMutex;
std::unordered_map<std::string, std::unique_ptr<Logger>> g_loggers;
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
  std::lock_guard<std::mutex> lock(g_loggerMutex);
  const auto existing = g_loggers.find(name);
  if(existing != g_loggers.end()) {
    return *existing->second;
  }
  auto logger = std::make_unique<Logger>();
  Logger& reference = *logger;
  g_loggers.emplace(name, std::move(logger));
  return reference;
}
void Logger::setUseParentHandlers(bool value) {
  useParentHandlers_ = value;
}
void Logger::addHandler(std::unique_ptr<LogHandler> handler) {
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
  for(const std::unique_ptr<LogHandler>& handler : handlers_) {
    if(handler != nullptr) {
      handler->publish(record);
    }
  }
}
Logger& ServerLog::LOGGER = Logger::getLogger("Minecraft");
void ServerLog::init() {
  const std::shared_ptr<ConsoleFormatter> consoleFormatter = std::make_shared<ConsoleFormatter>();
  LOGGER.setUseParentHandlers(false);
  auto consoleHandler = std::make_unique<ConsoleLogHandler>();
  consoleHandler->setFormatter(consoleFormatter);
  LOGGER.addHandler(std::move(consoleHandler));
  try {
    auto fileHandler = std::make_unique<FileLogHandler>("server.log", true);
    fileHandler->setFormatter(consoleFormatter);
    LOGGER.addHandler(std::move(fileHandler));
  } catch(const std::exception& exception) {
    LOGGER.log(LogLevel::Warning, "Failed to log to server.log", &exception);
  }
}
} // namespace net::minecraft::server
