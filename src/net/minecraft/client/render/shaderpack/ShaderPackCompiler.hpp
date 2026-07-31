#pragma once
#include <functional>
#include <string>
namespace net::minecraft::client::gl {
class ShaderProgram;
}
namespace net::minecraft::client::render::shaderpack {
class ShaderPackInstance;
class ShaderPackCompiler {
 public:
 static std::string readText(const ShaderPackInstance& pack, const std::string& path);
 static const std::string& cachedText(ShaderPackInstance& pack, const std::string& path);
 static std::string resolveIncludes(ShaderPackInstance& pack, const std::string& path);
 static gl::ShaderProgram* compile(ShaderPackInstance& pack, const std::string& programName,
                                   const std::function<void(ShaderPackInstance&, const std::string&)>& logOnce);
};
} // namespace net::minecraft::client::render::shaderpack
