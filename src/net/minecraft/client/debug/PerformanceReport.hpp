#pragma once
#include <filesystem>
#include <string>
#include "net/minecraft/client/debug/RenderProfiler.hpp"
#include "net/minecraft/util/logging/Logging.hpp"
namespace net::minecraft::client::debug {
struct PerformanceReportMetadata {
 std::string shaderPack;
 std::string world;
 int width = 0;
 int height = 0;
 int warmupFrames = 0;
 int captureSeconds = 0;
};
[[nodiscard]] bool writePerformanceReport(
    const std::filesystem::path& path,
    const PerformanceReportMetadata& metadata,
    const RenderProfileSnapshot& profile,
    const net::minecraft::util::logging::LogDispatcherStats& loggingStart,
    const net::minecraft::util::logging::LogDispatcherStats& loggingEnd);
} // namespace net::minecraft::client::debug
