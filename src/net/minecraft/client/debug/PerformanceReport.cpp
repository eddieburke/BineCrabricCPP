#include "net/minecraft/client/debug/PerformanceReport.hpp"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <iomanip>
namespace net::minecraft::client::debug {
namespace {
std::string jsonString(const std::string& value) {
 std::string escaped;
 escaped.reserve(value.size() + 2);
 escaped.push_back('"');
 for(const unsigned char ch : value) {
  switch(ch) {
  case '"': escaped += "\\\""; break;
  case '\\': escaped += "\\\\"; break;
  case '\n': escaped += "\\n"; break;
  case '\r': escaped += "\\r"; break;
  case '\t': escaped += "\\t"; break;
  default:
   if(ch < 0x20) {
    char buffer[7]{};
    std::snprintf(buffer, sizeof(buffer), "\\u%04x", static_cast<unsigned>(ch));
    escaped += buffer;
   } else {
    escaped.push_back(static_cast<char>(ch));
   }
  }
 }
 escaped.push_back('"');
 return escaped;
}
double milliseconds(double nanos) {
 return nanos / 1.0e6;
}
std::uint64_t delta(std::uint64_t end, std::uint64_t start) {
 return end - std::min(end, start);
}
} // namespace
bool writePerformanceReport(
    const std::filesystem::path& path,
    const PerformanceReportMetadata& metadata,
    const RenderProfileSnapshot& profile,
    const net::minecraft::util::logging::LogDispatcherStats& loggingStart,
    const net::minecraft::util::logging::LogDispatcherStats& loggingEnd) {
 try {
  if(path.has_parent_path()) {
   std::filesystem::create_directories(path.parent_path());
  }
  std::ofstream output(path, std::ios::binary | std::ios::trunc);
  if(!output) {
   return false;
  }
  const std::uint64_t logsEnqueued = delta(loggingEnd.enqueued, loggingStart.enqueued);
  const std::uint64_t logsWritten = delta(loggingEnd.written, loggingStart.written);
  const std::uint64_t logsDropped = delta(loggingEnd.dropped, loggingStart.dropped);
  const std::uint64_t logEnqueueNanos = delta(loggingEnd.enqueueCpuNanos, loggingStart.enqueueCpuNanos);
  const std::uint64_t logWriterNanos = delta(loggingEnd.writerCpuNanos, loggingStart.writerCpuNanos);
  const bool contaminated = logsDropped != 0;
  const auto generatedAt = std::chrono::duration_cast<std::chrono::milliseconds>(
                               std::chrono::system_clock::now().time_since_epoch())
                               .count();
  output << std::fixed << std::setprecision(6);
  output << "{\n";
  output << "  \"schema\": 2,\n";
  output << "  \"generated_at_unix_ms\": " << generatedAt << ",\n";
  output << "  \"shader_pack\": " << jsonString(metadata.shaderPack) << ",\n";
  output << "  \"world\": " << jsonString(metadata.world) << ",\n";
  output << "  \"resolution\": {\"width\": " << metadata.width << ", \"height\": "
         << metadata.height << "},\n";
  output << "  \"warmup_frames\": " << metadata.warmupFrames << ",\n";
  output << "  \"capture_seconds\": " << metadata.captureSeconds << ",\n";
  output << "  \"frames\": " << profile.frames << ",\n";
  output << "  \"contaminated\": " << (contaminated ? "true" : "false") << ",\n";
  output << "  \"contamination_reasons\": "
         << (contaminated ? "[\"logging_queue_overflow\"]" : "[]") << ",\n";
  output << "  \"instrumentation\": {\"gpu_query_interval_frames\": "
         << RenderProfiler::gpuQueryInterval() << ", \"gpu_frame_samples\": " << profile.gpuFrames
         << ", \"percentile_history_frames\": " << profile.frameHistorySamples
         << ", \"cpu_span_probes\": " << profile.cpuSpanProbes
         << ", \"metric_writes\": " << profile.metricWrites
         << ", \"gpu_timestamp_writes\": " << profile.gpuTimestampWrites << "},\n";
  output << "  \"frame_ms\": {\"mean\": " << milliseconds(profile.frameAverageNs)
         << ", \"min\": " << milliseconds(profile.frameMinNs)
         << ", \"max\": " << milliseconds(profile.frameMaxNs)
         << ", \"p50\": " << milliseconds(profile.frameP50Ns)
         << ", \"p95\": " << milliseconds(profile.frameP95Ns)
         << ", \"p99\": " << milliseconds(profile.frameP99Ns)
         << ", \"gpu_mean\": " << milliseconds(profile.gpuFrameAverageNs) << "},\n";
  output << "  \"spans\": [\n";
  for(std::size_t index = 0; index < profile.spans.size(); ++index) {
   const RenderSpanSnapshot& span = profile.spans[index];
   output << "    {\"name\": " << jsonString(span.name)
          << ", \"cpu_ms\": " << milliseconds(span.cpuAverageNs)
          << ", \"gpu_ms\": " << milliseconds(span.gpuAverageNs)
          << ", \"cpu_samples\": " << span.cpuSamples
          << ", \"gpu_samples\": " << span.gpuSamples << "}";
   output << (index + 1 == profile.spans.size() ? "\n" : ",\n");
  }
  output << "  ],\n";
  output << "  \"metrics\": {\n";
  for(int metric = 0; metric < kRenderMetricCount; ++metric) {
   output << "    " << jsonString(RenderProfiler::metricName(static_cast<RenderMetric>(metric))) << ": "
          << profile.metricAverages[static_cast<std::size_t>(metric)];
   output << (metric + 1 == kRenderMetricCount ? "\n" : ",\n");
  }
  output << "  },\n";
  output << "  \"logging\": {\"queued_at_start\": " << loggingStart.queued
         << ", \"queued_at_end\": " << loggingEnd.queued
         << ", \"lifetime_max_queue_depth\": " << loggingEnd.maxQueueDepth
         << ", \"enqueued_during_capture\": " << logsEnqueued
         << ", \"written_during_capture\": " << logsWritten
         << ", \"dropped_during_capture\": " << logsDropped
         << ", \"enqueue_cpu_ms\": " << milliseconds(static_cast<double>(logEnqueueNanos))
         << ", \"writer_cpu_ms\": " << milliseconds(static_cast<double>(logWriterNanos)) << "}\n";
  output << "}\n";
  output.flush();
  return static_cast<bool>(output);
 } catch(...) {
  return false;
 }
}
} // namespace net::minecraft::client::debug
