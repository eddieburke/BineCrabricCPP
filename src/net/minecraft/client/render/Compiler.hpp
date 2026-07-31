#pragma once
#include <functional>
#include <string>
#include "net/minecraft/util/logging/Logging.hpp"
namespace net::minecraft::client::gl {
class ShaderProgram;
}
namespace net::minecraft::client::render::shaderpack {
class ShaderPackInstance;
class ShaderPackCompiler {
 public:
 using LogFn = std::function<void(ShaderPackInstance&, const std::string&)>;
 using LogFnLevel = std::function<void(ShaderPackInstance&, const std::string&,
                                       ::net::minecraft::util::logging::LogLevel)>;
 static std::string readText(const ShaderPackInstance& pack, const std::string& path);
 static const std::string& cachedText(ShaderPackInstance& pack, const std::string& path);
 static std::string resolveIncludes(ShaderPackInstance& pack, const std::string& path);
 static gl::ShaderProgram* compile(ShaderPackInstance& pack, const std::string& programName,
                                   const LogFn& logOnce);
 static gl::ShaderProgram* compile(ShaderPackInstance& pack, const std::string& programName,
                                   const LogFnLevel& logOnce);
};
} // namespace net::minecraft::client::render::shaderpack

namespace net::minecraft::client::render {
using PackCompiler = shaderpack::ShaderPackCompiler;
} // namespace net::minecraft::client::render
