#pragma once
#include <functional>
#include <string>
#include "net/minecraft/util/logging/Logging.hpp"
namespace net::minecraft::client::gl {
class ShaderProgram;
}
namespace net::minecraft::client::render {
class PackInstance;
class PackCompiler {
 public:
 using LogFn = std::function<void(PackInstance&, const std::string&)>;
 using LogFnLevel = std::function<void(PackInstance&, const std::string&,
                                       ::net::minecraft::util::logging::LogLevel)>;
 static std::string readText(const PackInstance& pack, const std::string& path);
 static const std::string& cachedText(PackInstance& pack, const std::string& path);
 static std::string resolveIncludes(PackInstance& pack, const std::string& path);
  static gl::ShaderProgram* compile(PackInstance& pack, const std::string& programName,
                                    const LogFn& logOnce);
  static gl::ShaderProgram* compile(PackInstance& pack, const std::string& programName,
                                    const LogFnLevel& logOnce);
  // Kick off asynchronous worker compilation of every enabled program in the pack
  // without waiting. Call once after a pack loads (or its settings change) so the
  // first frame that needs a program finds it compiled or completing in the
  // background. No-op when the compile service is not running.
  static void prewarm(PackInstance& pack, const LogFnLevel& logOnce);
  static bool validate(PackInstance& pack, const LogFnLevel& logOnce);

 private:
  static gl::ShaderProgram* compileImpl(PackInstance& pack, const std::string& programName,
                                        const LogFn& logOnce);
};
} // namespace net::minecraft::client::render
