#include "net/minecraft/client/gl/ShaderProgram.hpp"
#include <utility>
#include "net/minecraft/client/ClientLog.hpp"
#include "net/minecraft/client/gl/GLCore.hpp"
#include "net/minecraft/util/math/Matrix4f.hpp"
namespace net::minecraft::client::gl {
namespace {
// GL enum literals kept local so this file does not depend on the (undef-heavy)
// fixed-function constant headers.
constexpr unsigned int kVertexShader = 0x8B31;
constexpr unsigned int kFragmentShader = 0x8B30;
constexpr unsigned int kCompileStatus = 0x8B81;
constexpr unsigned int kLinkStatus = 0x8B82;
// Builds a stage source string: version preamble, then defines, then the body.
std::string assemble(const std::string& versionPreamble,
                     const std::vector<ShaderDefine>& defines,
                     const std::string& body) {
  std::string out;
  out.reserve(versionPreamble.size() + body.size() + 64 * defines.size() + 64);
  out += versionPreamble;
  if(!out.empty() && out.back() != '\n') {
    out += '\n';
  }
  for(const ShaderDefine& def : defines) {
    out += "#define ";
    out += def.name;
    if(!def.value.empty()) {
      out += ' ';
      out += def.value;
    }
    out += '\n';
  }
  out += body;
  return out;
}
unsigned int compileStage(unsigned int type, const std::string& source, std::string& errorOut) {
  const unsigned int shader = GLCore::createShader(type);
  if(shader == 0) {
    errorOut = "glCreateShader returned 0";
    return 0;
  }
  const char* bytes = source.c_str();
  GLCore::shaderSource(shader, 1, &bytes, nullptr);
  GLCore::compileShader(shader);
  int success = 0;
  GLCore::getShaderiv(shader, kCompileStatus, &success);
  if(success == 0) {
    char log[2048]{};
    GLCore::getShaderInfoLog(shader, sizeof(log), nullptr, log);
    errorOut = log;
    GLCore::deleteShader(shader);
    return 0;
  }
  return shader;
}
} // namespace
ShaderProgram::~ShaderProgram() {
  destroy();
}
ShaderProgram::ShaderProgram(ShaderProgram&& other) noexcept
    : program_(other.program_),
      uniformCache_(std::move(other.uniformCache_)),
      lastError_(std::move(other.lastError_)) {
  other.program_ = 0;
}
ShaderProgram& ShaderProgram::operator=(ShaderProgram&& other) noexcept {
  if(this != &other) {
    destroy();
    program_ = other.program_;
    uniformCache_ = std::move(other.uniformCache_);
    lastError_ = std::move(other.lastError_);
    other.program_ = 0;
  }
  return *this;
}
bool ShaderProgram::supported() {
  GLCore::ensureLoaded();
  return GLCore::shaderSupported;
}
bool ShaderProgram::compile(const std::string& vertexSource,
                            const std::string& fragmentSource,
                            const std::string& versionPreamble,
                            const std::vector<ShaderDefine>& defines) {
  GLCore::ensureLoaded();
  destroy();
  lastError_.clear();
  if(!GLCore::shaderSupported) {
    lastError_ = "shader entry points unavailable";
    return false;
  }
  const std::string vsrc = assemble(versionPreamble, defines, vertexSource);
  const std::string fsrc = assemble(versionPreamble, defines, fragmentSource);
  const unsigned int vertex = compileStage(kVertexShader, vsrc, lastError_);
  if(vertex == 0) {
    ClientLog::LOGGER.log(LogLevel::Warning, "[shader] vertex compile failed:\n" + lastError_);
    return false;
  }
  const unsigned int fragment = compileStage(kFragmentShader, fsrc, lastError_);
  if(fragment == 0) {
    ClientLog::LOGGER.log(LogLevel::Warning, "[shader] fragment compile failed:\n" + lastError_);
    GLCore::deleteShader(vertex);
    return false;
  }
  const unsigned int program = GLCore::createProgram();
  GLCore::attachShader(program, vertex);
  GLCore::attachShader(program, fragment);
  // Fixed attribute locations so a single VAO layout works across all programs.
  if(GLCore::bindAttribLocation != nullptr) {
    GLCore::bindAttribLocation(program, 0, "aPos");
    GLCore::bindAttribLocation(program, 1, "aUV");
    GLCore::bindAttribLocation(program, 2, "aColor");
    GLCore::bindAttribLocation(program, 3, "aNormal");
  }
  GLCore::linkProgram(program);
  GLCore::deleteShader(vertex);
  GLCore::deleteShader(fragment);
  int success = 0;
  GLCore::getProgramiv(program, kLinkStatus, &success);
  if(success == 0) {
    char log[2048]{};
    GLCore::getProgramInfoLog(program, sizeof(log), nullptr, log);
    lastError_ = log;
    ClientLog::LOGGER.log(LogLevel::Warning, "[shader] link failed:\n" + lastError_);
    GLCore::deleteProgram(program);
    return false;
  }
  program_ = program;
  uniformCache_.clear();
  return true;
}
void ShaderProgram::destroy() {
  if(program_ != 0 && GLCore::deleteProgram != nullptr) {
    GLCore::deleteProgram(program_);
  }
  program_ = 0;
  uniformCache_.clear();
}
void ShaderProgram::bind() const {
  if(program_ != 0 && GLCore::useProgram != nullptr) {
    GLCore::useProgram(program_);
  }
}
void ShaderProgram::unbind() {
  if(GLCore::useProgram != nullptr) {
    GLCore::useProgram(0);
  }
}
int ShaderProgram::location(const std::string& name) const {
  if(program_ == 0 || GLCore::getUniformLocation == nullptr) {
    return -1;
  }
  const auto found = uniformCache_.find(name);
  if(found != uniformCache_.end()) {
    return found->second;
  }
  const int loc = GLCore::getUniformLocation(program_, name.c_str());
  uniformCache_.emplace(name, loc);
  return loc;
}
void ShaderProgram::set1i(const std::string& name, int value) const {
  const int loc = location(name);
  if(loc >= 0 && GLCore::uniform1i != nullptr) {
    GLCore::uniform1i(loc, value);
  }
}
void ShaderProgram::set1f(const std::string& name, float value) const {
  const int loc = location(name);
  if(loc >= 0 && GLCore::uniform1f != nullptr) {
    GLCore::uniform1f(loc, value);
  }
}
void ShaderProgram::set2f(const std::string& name, float x, float y) const {
  const int loc = location(name);
  if(loc >= 0 && GLCore::uniform2f != nullptr) {
    GLCore::uniform2f(loc, x, y);
  }
}
void ShaderProgram::set3f(const std::string& name, float x, float y, float z) const {
  const int loc = location(name);
  if(loc >= 0 && GLCore::uniform3f != nullptr) {
    GLCore::uniform3f(loc, x, y, z);
  }
}
void ShaderProgram::set4f(const std::string& name, float x, float y, float z, float w) const {
  const int loc = location(name);
  if(loc >= 0 && GLCore::uniform4f != nullptr) {
    GLCore::uniform4f(loc, x, y, z, w);
  }
}
void ShaderProgram::setMatrix4(const std::string& name, const float* value, bool transpose) const {
  const int loc = location(name);
  if(loc >= 0 && GLCore::uniformMatrix4fv != nullptr) {
    GLCore::uniformMatrix4fv(loc, 1, transpose ? 1 : 0, value);
  }
}
void ShaderProgram::setMatrix4(const std::string& name,
                               const net::minecraft::util::math::Matrix4f& value) const {
  setMatrix4(name, value.data(), false);
}
} // namespace net::minecraft::client::gl
