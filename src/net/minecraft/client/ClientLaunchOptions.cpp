#include "net/minecraft/client/ClientLaunchOptions.hpp"
#include <charconv>
#include <limits>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <vector>
namespace net::minecraft::client {
namespace {
template <typename Integer>
Integer parseInteger(std::string_view value, std::string_view option, Integer minimum) {
 Integer parsed = 0;
 const char* first = value.data();
 const char* last = value.data() + value.size();
 const auto result = std::from_chars(first, last, parsed);
 if(result.ec != std::errc{} || result.ptr != last || parsed < minimum) {
  throw std::runtime_error(std::string(option) + " expects an integer of at least " +
                           std::to_string(minimum) + ", got '" + std::string(value) + "'");
 }
 return parsed;
}
} // namespace
ClientLaunchOptions parseClientLaunchOptions(int argc,
                                             const char* const* argv,
                                             std::string defaultUsername) {
 ClientLaunchOptions launch;
 launch.username = std::move(defaultUsername);
 std::vector<std::string> positional;
 for(int i = 1; i < argc; ++i) {
  if(argv[i] == nullptr) {
   continue;
  }
  const std::string argument = argv[i];
  const auto nextValue = [&]() -> std::string {
   if(i + 1 >= argc || argv[i + 1] == nullptr) {
    throw std::runtime_error(argument + " expects a value");
   }
   return argv[++i];
  };
  if(argument == "--world") {
   launch.startup.world = nextValue();
  } else if(argument == "--seed") {
   launch.startup.seed = parseInteger<std::int64_t>(nextValue(), argument,
                                                    std::numeric_limits<std::int64_t>::min());
  } else if(argument == "--username") {
   launch.username = nextValue();
  } else if(argument == "--session") {
   launch.sessionId = nextValue();
  } else if(argument == "--server") {
   launch.server = nextValue();
  } else if(argument == "--debug-hud") {
   launch.startup.debugHud = true;
  } else if(argument == "--perf-trace") {
   launch.startup.perfTraceSeconds = parseInteger<int>(nextValue(), argument, 1);
  } else if(argument == "--shader-benchmark") {
   launch.startup.perfTraceSeconds = parseInteger<int>(nextValue(), argument, 1);
   launch.startup.headless = true;
  } else if(argument == "--benchmark-frames") {
   launch.startup.benchmarkFrames = parseInteger<int>(nextValue(), argument, 1);
   launch.startup.headless = true;
  } else if(argument == "--perf-warmup") {
   launch.startup.perfWarmupFrames = parseInteger<int>(nextValue(), argument, 0);
  } else if(argument == "--perf-output") {
   const std::string output = nextValue();
   if(output.empty()) {
    throw std::runtime_error("--perf-output expects a file path");
   }
   launch.startup.perfOutput = output;
  } else if(argument == "--headless") {
   launch.startup.headless = true;
  } else if(argument == "--width") {
   launch.startup.width = parseInteger<int>(nextValue(), argument, 1);
  } else if(argument == "--height") {
   launch.startup.height = parseInteger<int>(nextValue(), argument, 1);
  } else if(argument.starts_with("--")) {
   throw std::runtime_error("Unknown option '" + argument + "'");
  } else {
   positional.push_back(argument);
  }
 }
 if(!positional.empty()) {
  launch.username = positional[0];
 }
 if(positional.size() > 1) {
  launch.sessionId = positional[1];
 }
 if(positional.size() > 2) {
  launch.server = positional[2];
 }
 if(positional.size() > 3) {
  throw std::runtime_error("Too many positional arguments");
 }
 if(!launch.startup.perfOutput.empty() && launch.startup.perfOutput.filename().empty()) {
  throw std::runtime_error("--perf-output expects a file path");
 }
 const bool automaticBenchmark = launch.startup.headless &&
                                 (launch.startup.perfTraceSeconds > 0 || launch.startup.benchmarkFrames > 0);
 if(automaticBenchmark && launch.startup.world.empty() && !launch.server.has_value()) {
  throw std::runtime_error("A headless benchmark requires --world or --server");
 }
 return launch;
}
} // namespace net::minecraft::client
