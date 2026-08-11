#include <gtest/gtest.h>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <sstream>
#include "net/minecraft/client/debug/PerformanceReport.hpp"
namespace net::minecraft::test {
TEST(PerformanceReportTest, MarksLoggingOverflowAsMeasurementContamination) {
 const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
 const auto path = std::filesystem::temp_directory_path() /
                   ("minecraft-performance-report-" + std::to_string(stamp) + ".json");
 client::debug::PerformanceReportMetadata metadata;
 metadata.shaderPack = "Pack\nName";
 client::debug::RenderProfileSnapshot profile;
 profile.frames = 60;
 util::logging::LogDispatcherStats start;
 start.dropped = 4;
 util::logging::LogDispatcherStats end;
 end.dropped = 5;
 end.enqueued = 10;
 end.written = 9;
 ASSERT_TRUE(client::debug::writePerformanceReport(path, metadata, profile, start, end));
 std::ifstream input(path, std::ios::binary);
 std::ostringstream contents;
 contents << input.rdbuf();
 EXPECT_NE(contents.str().find("\"contaminated\": true"), std::string::npos);
 EXPECT_NE(contents.str().find("logging_queue_overflow"), std::string::npos);
 EXPECT_NE(contents.str().find("Pack\\nName"), std::string::npos);
 std::error_code error;
 std::filesystem::remove(path, error);
}
} // namespace net::minecraft::test
