#pragma once
#include <cstddef>
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
 using LogFnLevel = std::function<void(PackInstance&, const std::string&,
                                       ::net::minecraft::util::logging::LogLevel)>;
 static std::string readText(const PackInstance& pack, const std::string& path);
 static const std::string& cachedText(PackInstance& pack, const std::string& path);
 static std::string resolveIncludes(PackInstance& pack, const std::string& path);
 static gl::ShaderProgram* compile(PackInstance& pack, const std::string& programName,
                                   const LogFnLevel& logOnce);
};
} // namespace net::minecraft::client::render
