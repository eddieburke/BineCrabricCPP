#include <gtest/gtest.h>
#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include "net/minecraft/util/logging/Logging.hpp"
namespace net::minecraft::test {
namespace {
class CountingLogHandler final : public util::logging::LogHandler {
 public:
 void publish(const util::logging::LogRecord&) override {
  std::lock_guard<std::mutex> lock(mutex_);
  ++count_;
  cv_.notify_all();
 }
 bool waitFor(std::size_t count) {
  std::unique_lock<std::mutex> lock(mutex_);
  return cv_.wait_for(lock, std::chrono::seconds(2), [&] { return count_ >= count; });
 }

 private:
 std::mutex mutex_;
 std::condition_variable cv_;
 std::size_t count_ = 0;
};
} // namespace
TEST(LoggingDispatcherTest, ReportsQueueAndWriterCostWithoutSynchronousPublishing) {
 auto& dispatcher = util::logging::LogDispatcher::instance();
 dispatcher.start();
 auto& logger = util::logging::Logger::getLogger("logging-dispatcher-perf-test");
 auto handler = std::make_unique<CountingLogHandler>();
 CountingLogHandler* observed = handler.get();
 logger.addHandler(std::move(handler));
 const auto before = dispatcher.stats();
 constexpr std::size_t records = 64;
 for(std::size_t index = 0; index < records; ++index) {
  logger.info("record");
 }
 ASSERT_TRUE(observed->waitFor(records));
 const auto after = dispatcher.stats();
 EXPECT_GE(after.enqueued - before.enqueued, records);
 EXPECT_GE(after.written - before.written, records);
 EXPECT_GT(after.enqueueCpuNanos, before.enqueueCpuNanos);
 EXPECT_GT(after.writerCpuNanos, before.writerCpuNanos);
 EXPECT_LE(after.maxQueueDepth, 16384U);
}
} // namespace net::minecraft::test
