#include "net/minecraft/client/gl/ShaderProgram.hpp"
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <string_view>
#include <utility>
#include "net/minecraft/client/gl/GLCore.hpp"
#include "net/minecraft/client/gl/GlConstants.hpp"
#include "net/minecraft/client/render/VertexAbi.hpp"
#include "net/minecraft/util/math/Matrix4f.hpp"
namespace net::minecraft::client::gl {
// Last program passed to glUseProgram (0 = none). Lets bind() skip redundant
// glUseProgram calls and unbind()/destroy() reset the cached state.
thread_local unsigned int s_lastBoundProgram = 0;
namespace {
// GL enum literals kept local so this file does not depend on the (undef-heavy)
// fixed-function constant headers.
constexpr unsigned int kVertexShader = 0x8B31;
constexpr unsigned int kFragmentShader = 0x8B30;
constexpr unsigned int kGeometryShader = 0x8DD9;
constexpr unsigned int kTessControlShader = 0x8E88;
constexpr unsigned int kTessEvaluationShader = 0x8E87;
constexpr unsigned int kComputeShader = 0x91B9;
constexpr unsigned int kCompileStatus = 0x8B81;
constexpr unsigned int kLinkStatus = 0x8B82;
bool corePreambleAtLeast(std::string_view source, int minimumVersion) {
 std::size_t cursor = 0;
 while(cursor < source.size() && std::isspace(static_cast<unsigned char>(source[cursor]))) ++cursor;
 constexpr std::string_view directive = "#version";
 if(!source.substr(cursor).starts_with(directive)) return false;
 cursor += directive.size();
 if(cursor >= source.size() || !std::isspace(static_cast<unsigned char>(source[cursor]))) return false;
 while(cursor < source.size() && std::isspace(static_cast<unsigned char>(source[cursor]))) ++cursor;
 int version = 0;
 const std::size_t digits = cursor;
 while(cursor < source.size() && std::isdigit(static_cast<unsigned char>(source[cursor]))) {
  version = version * 10 + source[cursor] - '0';
  ++cursor;
 }
 if(cursor == digits || version < minimumVersion) return false;
 while(cursor < source.size() && (source[cursor] == ' ' || source[cursor] == '\t')) ++cursor;
 constexpr std::string_view profile = "core";
 if(!source.substr(cursor).starts_with(profile)) return false;
 cursor += profile.size();
 return cursor >= source.size() || std::isspace(static_cast<unsigned char>(source[cursor]));
}
std::string stripLeadingVersionDirective(const std::string& body) {
 // If the body starts with a #version directive, strip it so the caller-supplied
 // preamble is not duplicated.
 const char* p = body.c_str();
 // skip leading whitespace
 while(*p == ' ' || *p == '\t') {
  ++p;
 }
 if(body.compare(static_cast<std::size_t>(p - body.c_str()), 9, "#version ", 0, 9) != 0) {
  return body;
 }
 // find end of the #version line
 const char* eol = p;
 while(*eol != '\0' && *eol != '\n') {
  ++eol;
 }
 while(*eol == '\n') {
  ++eol;
 }
 return std::string(eol);
}
std::string assemble(const std::string& versionPreamble,
                     const std::string& body) {
 std::string stripped = stripLeadingVersionDirective(body);
 std::string out;
 out.reserve(versionPreamble.size() + stripped.size() + 1);
 out += versionPreamble;
 if(!out.empty() && out.back() != '\n') {
  out += '\n';
 }
 out += stripped;
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
      samplerKinds_(std::move(other.samplerKinds_)),
      samplerNames_(std::move(other.samplerNames_)),
      tessellation_(other.tessellation_),
      drawBuffers_(std::move(other.drawBuffers_)),
      drawBufferColortexIndices_(std::move(other.drawBufferColortexIndices_)),
      lastError_(std::move(other.lastError_)) {
 other.program_ = 0;
}
ShaderProgram& ShaderProgram::operator=(ShaderProgram&& other) noexcept {
 if(this != &other) {
  destroy();
  program_ = other.program_;
  uniformCache_ = std::move(other.uniformCache_);
  samplerKinds_ = std::move(other.samplerKinds_);
  samplerNames_ = std::move(other.samplerNames_);
  tessellation_ = other.tessellation_;
  drawBuffers_ = std::move(other.drawBuffers_);
  drawBufferColortexIndices_ = std::move(other.drawBufferColortexIndices_);
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
                            const std::string& geometrySource,
                            const std::string& tessControlSource,
                            const std::string& tessEvaluationSource) {
 destroy();
 lastError_.clear();
 if(!corePreambleAtLeast(versionPreamble, 330)) {
  lastError_ = "raster shaders require #version 330 core or newer";
  return false;
 }
 GLCore::ensureLoaded();
 if(!GLCore::shaderSupported) {
  lastError_ = "shader entry points unavailable";
  return false;
 }
 const std::string vsrc = assemble(versionPreamble, vertexSource);
 const std::string fsrc = assemble(versionPreamble, fragmentSource);
 const std::string gsrc = geometrySource.empty() ? std::string{} : assemble(versionPreamble, geometrySource);
 const std::string tcsrc = tessControlSource.empty() ? std::string{} : assemble(versionPreamble, tessControlSource);
 const std::string tesrc = tessEvaluationSource.empty() ? std::string{} : assemble(versionPreamble, tessEvaluationSource);
 std::vector<unsigned int> shaders;
 const auto add = [&](unsigned int type, const std::string& source) {
  if(source.empty()) return true;
  const unsigned int shader = compileStage(type, source, lastError_);
  if(shader == 0) return false;
  shaders.push_back(shader);
  return true;
 };
 if(!add(kVertexShader, vsrc) || !add(kTessControlShader, tcsrc) || !add(kTessEvaluationShader, tesrc) ||
    !add(kGeometryShader, gsrc) || !add(kFragmentShader, fsrc)) {
  for(unsigned int shader : shaders) GLCore::deleteShader(shader);
  return false;
 }
 const unsigned int program = GLCore::createProgram();
 for(unsigned int shader : shaders) GLCore::attachShader(program, shader);
 if(GLCore::bindAttribLocation != nullptr) {
  for(const auto& binding : render::vertex_abi::Bindings)
   GLCore::bindAttribLocation(program, binding.location, binding.name);
 }
 GLCore::linkProgram(program);
 for(unsigned int shader : shaders) GLCore::deleteShader(shader);
 int success = 0;
 GLCore::getProgramiv(program, kLinkStatus, &success);
 if(success == 0) {
  char log[2048]{};
  GLCore::getProgramInfoLog(program, sizeof(log), nullptr, log);
  lastError_ = log;
  GLCore::deleteProgram(program);
  return false;
 }
 program_ = program;
 uniformCache_.clear();
 samplerKinds_.clear();
 samplerNames_.clear();
 tessellation_ = !tessControlSource.empty() && !tessEvaluationSource.empty();
 reflectSamplers();
 return true;
}
bool ShaderProgram::compileCompute(const std::string& computeSource,
                                   const std::string& versionPreamble) {
 destroy();
 lastError_.clear();
 if(!corePreambleAtLeast(versionPreamble, 430)) {
  lastError_ = "compute shaders require #version 430 core or newer";
  return false;
 }
 GLCore::ensureLoaded();
 if(!GLCore::computeSupported) {
  lastError_ = "compute shader entry points unavailable (needs GL 4.3 core)";
  return false;
 }
 const std::string csrc = assemble(versionPreamble, computeSource);
 const unsigned int stage = compileStage(kComputeShader, csrc, lastError_);
 if(stage == 0) {
  return false;
 }
 const unsigned int program = GLCore::createProgram();
 GLCore::attachShader(program, stage);
 GLCore::linkProgram(program);
 GLCore::deleteShader(stage);
 int success = 0;
 GLCore::getProgramiv(program, kLinkStatus, &success);
 if(success == 0) {
  char log[2048]{};
  GLCore::getProgramInfoLog(program, sizeof(log), nullptr, log);
  lastError_ = log;
  GLCore::deleteProgram(program);
  return false;
 }
 program_ = program;
 uniformCache_.clear();
 samplerKinds_.clear();
 samplerNames_.clear();
 tessellation_ = false;
 reflectSamplers();
 return true;
}
void ShaderProgram::destroy() {
 if(program_ != 0 && program_ == s_lastBoundProgram) s_lastBoundProgram = 0;
 if(program_ != 0 && GLCore::deleteProgram != nullptr) {
  GLCore::deleteProgram(program_);
 }
 program_ = 0;
 uniformCache_.clear();
 samplerKinds_.clear();
 samplerNames_.clear();
 drawBuffers_.clear();
 drawBufferColortexIndices_.clear();
 tessellation_ = false;
}
void ShaderProgram::bind() const {
 if(program_ != 0 && GLCore::useProgram != nullptr) {
  if(program_ == s_lastBoundProgram) return;
  s_lastBoundProgram = program_;
  GLCore::useProgram(program_);
 }
}
void ShaderProgram::unbind() {
 if(GLCore::useProgram != nullptr) {
  s_lastBoundProgram = 0;
  GLCore::useProgram(0);
 }
}
void ShaderProgram::setDrawBufferColortexIndices(const std::vector<int>& colortexIndices) {
 constexpr unsigned int kColorAttachment0 = 0x8CE0;
 drawBufferColortexIndices_ = colortexIndices;
 drawBuffers_.clear();
 drawBuffers_.reserve(colortexIndices.size());
 for(int index : colortexIndices) {
  if(index < 0 || index >= 32) {
   continue;
  }
  drawBuffers_.push_back(kColorAttachment0 + static_cast<unsigned int>(index));
 }
}
void ShaderProgram::applyDrawBuffers(int colorAttachmentCount) const {
 constexpr unsigned int kColorAttachment0 = 0x8CE0;
 if(drawBuffers_.empty() || GLCore::drawBuffers == nullptr) {
  return;
 }
 int currentFbo = 0;
 ::glGetIntegerv(query::FramebufferBinding, &currentFbo);
 if(currentFbo == 0) {
  return;
 }
 const int maxAttachments = std::max(1, colorAttachmentCount);
 std::vector<unsigned int> attached;
 attached.reserve(drawBuffers_.size());
 for(unsigned int buffer : drawBuffers_) {
  const int index = static_cast<int>(buffer - kColorAttachment0);
  if(index >= 0 && index < maxAttachments) {
   attached.push_back(buffer);
  }
 }
 if(attached.empty()) {
  attached.push_back(kColorAttachment0);
 }
 GLCore::drawBuffers(static_cast<int>(attached.size()), attached.data());
}
int ShaderProgram::location(std::string_view name) const {
 if(program_ == 0 || GLCore::getUniformLocation == nullptr) {
  return -1;
 }
 const auto found = uniformCache_.find(name);
 if(found != uniformCache_.end()) {
  return found->second;
 }
 const int location = GLCore::getUniformLocation(program_, std::string(name).c_str());
 uniformCache_.emplace(std::string(name), location);
 return location;
}
void ShaderProgram::reflectSamplers() {
 if(GLCore::getActiveUniform == nullptr) return;
 int count = 0;
 int maxName = 0;
 GLCore::getProgramiv(program_, 0x8B86, &count);
 GLCore::getProgramiv(program_, 0x8B87, &maxName);
 std::vector<char> name(static_cast<std::size_t>(std::max(maxName, 1)));
 for(int index = 0; index < count; ++index) {
  int length = 0;
  int size = 0;
  unsigned int type = 0;
  GLCore::getActiveUniform(program_, static_cast<unsigned int>(index), maxName, &length, &size, &type, name.data());
  SamplerKind kind = SamplerKind::None;
  switch(type) {
  case 0x8B5F:
  case 0x8DCB:
  case 0x8DD3: kind = SamplerKind::Volume; break;
  case 0x8B61:
  case 0x8B62:
  case 0x8B64:
  case 0x8DC3:
  case 0x8DC4:
  case 0x8DC5:
  case 0x900D: kind = SamplerKind::Shadow; break;
  case 0x8DC9:
  case 0x8DCA:
  case 0x8DCC:
  case 0x8DCD:
  case 0x8DCE:
  case 0x8DCF:
  case 0x900E:
  case 0x9109:
  case 0x910C: kind = SamplerKind::Integer; break;
  case 0x8DD1:
  case 0x8DD2:
  case 0x8DD4:
  case 0x8DD5:
  case 0x8DD6:
  case 0x8DD7:
  case 0x8DD8:
  case 0x900F:
  case 0x910A:
  case 0x910D: kind = SamplerKind::Unsigned; break;
  case 0x8B5D:
  case 0x8B5E:
  case 0x8B60:
  case 0x8B63:
  case 0x8DC0:
  case 0x8DC1:
  case 0x8DC2:
  case 0x900C:
  case 0x9108:
  case 0x910B: kind = SamplerKind::Float; break;
  }
  if(kind == SamplerKind::None || length <= 0) continue;
  std::string uniform(name.data(), static_cast<std::size_t>(length));
  if(uniform.ends_with("[0]")) uniform.resize(uniform.size() - 3);
  samplerKinds_.emplace(uniform, kind);
  samplerNames_.push_back(std::move(uniform));
 }
}
void ShaderProgram::set1iAt(int loc, int value) const {
 if(loc >= 0 && GLCore::uniform1i != nullptr) {
  GLCore::uniform1i(loc, value);
 }
}
void ShaderProgram::set1fAt(int loc, float value) const {
 if(loc >= 0 && GLCore::uniform1f != nullptr) {
  GLCore::uniform1f(loc, value);
 }
}
void ShaderProgram::set2fAt(int loc, const float* values) const {
 if(loc >= 0 && GLCore::uniform2f != nullptr) {
  GLCore::uniform2f(loc, values[0], values[1]);
 }
}
void ShaderProgram::set3fAt(int loc, const float* values) const {
 if(loc >= 0 && GLCore::uniform3f != nullptr) {
  GLCore::uniform3f(loc, values[0], values[1], values[2]);
 }
}
void ShaderProgram::set4fAt(int loc, const float* values) const {
 if(loc >= 0 && GLCore::uniform4f != nullptr) {
  GLCore::uniform4f(loc, values[0], values[1], values[2], values[3]);
 }
}
void ShaderProgram::set2iAt(int loc, const int* values) const {
 if(loc >= 0 && GLCore::uniform2i != nullptr) GLCore::uniform2i(loc, values[0], values[1]);
}
void ShaderProgram::set3iAt(int loc, const int* values) const {
 if(loc >= 0 && GLCore::uniform3i != nullptr) GLCore::uniform3i(loc, values[0], values[1], values[2]);
}
void ShaderProgram::set4iAt(int loc, const int* values) const {
 if(loc >= 0 && GLCore::uniform4i != nullptr) GLCore::uniform4i(loc, values[0], values[1], values[2], values[3]);
}
void ShaderProgram::setMatrix3At(int loc, const float* values) const {
 if(loc >= 0 && GLCore::uniformMatrix3fv != nullptr) {
  GLCore::uniformMatrix3fv(loc, 1, 0, values);
 }
}
void ShaderProgram::setMatrix4At(int loc, const float* values) const {
 if(loc >= 0 && GLCore::uniformMatrix4fv != nullptr) {
  GLCore::uniformMatrix4fv(loc, 1, 0, values);
 }
}
ShaderProgram::SamplerKind ShaderProgram::samplerKind(std::string_view name) const {
 const auto found = samplerKinds_.find(name);
 return found == samplerKinds_.end() ? SamplerKind::None : found->second;
}
const std::vector<std::string>& ShaderProgram::declaredSamplers() const {
 return samplerNames_;
}
void ShaderProgram::set1i(std::string_view name, int value) const {
 const int loc = location(name);
 if(loc >= 0 && GLCore::uniform1i != nullptr) {
  GLCore::uniform1i(loc, value);
 }
}
void ShaderProgram::set1f(std::string_view name, float value) const {
 const int loc = location(name);
 if(loc >= 0 && GLCore::uniform1f != nullptr) {
  GLCore::uniform1f(loc, value);
 }
}
void ShaderProgram::set2f(std::string_view name, float x, float y) const {
 const int loc = location(name);
 if(loc >= 0 && GLCore::uniform2f != nullptr) {
  GLCore::uniform2f(loc, x, y);
 }
}
void ShaderProgram::set3f(std::string_view name, float x, float y, float z) const {
 const int loc = location(name);
 if(loc >= 0 && GLCore::uniform3f != nullptr) {
  GLCore::uniform3f(loc, x, y, z);
 }
}
void ShaderProgram::set4f(std::string_view name, float x, float y, float z, float w) const {
 const int loc = location(name);
 if(loc >= 0 && GLCore::uniform4f != nullptr) {
  GLCore::uniform4f(loc, x, y, z, w);
 }
}
void ShaderProgram::setMatrix3(std::string_view name, const float* value, bool transpose) const {
 const int loc = location(name);
 if(loc >= 0 && GLCore::uniformMatrix3fv != nullptr) {
  GLCore::uniformMatrix3fv(loc, 1, transpose ? 1 : 0, value);
 }
}
void ShaderProgram::setMatrix4(std::string_view name, const float* value, bool transpose) const {
 const int loc = location(name);
 if(loc >= 0 && GLCore::uniformMatrix4fv != nullptr) {
  GLCore::uniformMatrix4fv(loc, 1, transpose ? 1 : 0, value);
 }
}
void ShaderProgram::setMatrix4(std::string_view name, const net::minecraft::util::math::Matrix4f& value) const {
 setMatrix4(name, value.data(), false);
}

namespace {
using PFN_GetProgramBinary = void(APIENTRY*)(unsigned int, int, int*, unsigned int*, void*);
using PFN_ProgramBinary = void(APIENTRY*)(unsigned int, unsigned int, const void*, int);
using PFN_GetProgramivLocal = void(APIENTRY*)(unsigned int, unsigned int, int*);
constexpr unsigned int kProgramBinaryLength = 0x8741;

void* loadGlProc(const char* name) {
 PROC proc = wglGetProcAddress(name);
 if(proc == nullptr || proc == reinterpret_cast<PROC>(1) || proc == reinterpret_cast<PROC>(2) ||
    proc == reinterpret_cast<PROC>(3) || proc == reinterpret_cast<PROC>(-1)) {
  HMODULE module = GetModuleHandleA("opengl32.dll");
  return module != nullptr ? reinterpret_cast<void*>(GetProcAddress(module, name)) : nullptr;
 }
 return reinterpret_cast<void*>(proc);
}

struct BinaryProcs {
 PFN_GetProgramivLocal getProgramiv;
 PFN_GetProgramBinary getProgramBinary;
 PFN_ProgramBinary programBinary;
};

const BinaryProcs& binaryProcs() {
 static const BinaryProcs procs = {
     reinterpret_cast<PFN_GetProgramivLocal>(loadGlProc("glGetProgramiv")),
     reinterpret_cast<PFN_GetProgramBinary>(loadGlProc("glGetProgramBinary")),
     reinterpret_cast<PFN_ProgramBinary>(loadGlProc("glProgramBinary")),
 };
 return procs;
}

std::uint64_t mixHash(std::uint64_t h, const std::string& s) {
 // FNV-1a 64-bit over bytes, then avalanche into running hash.
 std::uint64_t x = 14695981039346656037ull;
 for(unsigned char c : s) {
  x ^= c;
  x *= 1099511628211ull;
 }
 h ^= x + 0x9e3779b97f4a7c15ull + (h << 6) + (h >> 2);
 return h;
}
} // namespace

std::uint64_t ShaderProgram::contentHash(bool compute,
                                         const std::string& preamble,
                                         const std::string& a,
                                         const std::string& b,
                                         const std::string& c,
                                         const std::string& d,
                                         const std::string& e,
                                         const std::string& abiSalt) {
 std::uint64_t h = compute ? 1ull : 0ull;
 h = mixHash(h, preamble);
 h = mixHash(h, a);
 h = mixHash(h, b);
 h = mixHash(h, c);
 h = mixHash(h, d);
 h = mixHash(h, e);
 h = mixHash(h, abiSalt);
 return h == 0 ? 1ull : h;
}

bool ShaderProgram::extractProgramBinary(ProgramBinaryBlob& out) {
 if(program_ == 0) {
  lastError_ = "no program";
  return false;
 }
 const auto& procs = binaryProcs();
 if(procs.getProgramiv == nullptr || procs.getProgramBinary == nullptr) {
  lastError_ = "glGetProgramBinary unavailable";
  return false;
 }
 auto getProgramiv = procs.getProgramiv;
 auto getProgramBinary = procs.getProgramBinary;
 int length = 0;
 getProgramiv(program_, kProgramBinaryLength, &length);
 if(length <= 0) {
  lastError_ = "empty program binary";
  return false;
 }
 out.bytes.resize(static_cast<std::size_t>(length));
 int written = 0;
 unsigned int format = 0;
 getProgramBinary(program_, length, &written, &format, out.bytes.data());
 if(written <= 0 || format == 0) {
  out.bytes.clear();
  lastError_ = "glGetProgramBinary failed";
  return false;
 }
 out.bytes.resize(static_cast<std::size_t>(written));
 out.binaryFormat = format;
 out.tessellation = tessellation_;
 out.flags = 0;
 if(out.compute) out.flags |= kFlagCompute;
 if(tessellation_) out.flags |= kFlagTessellation;
 return true;
}

bool ShaderProgram::compileToBinary(ProgramBinaryBlob& out,
                                    const std::string& vertexSource,
                                    const std::string& fragmentSource,
                                    const std::string& versionPreamble,
                                    const std::string& geometrySource,
                                    const std::string& tessControlSource,
                                    const std::string& tessEvaluationSource) {
 out = {};
 out.compute = false;
 if(!compile(vertexSource, fragmentSource, versionPreamble, geometrySource, tessControlSource,
             tessEvaluationSource)) {
  return false;
 }
 return extractProgramBinary(out);
}

bool ShaderProgram::compileComputeToBinary(ProgramBinaryBlob& out,
                                           const std::string& computeSource,
                                           const std::string& versionPreamble) {
 out = {};
 out.compute = true;
 if(!compileCompute(computeSource, versionPreamble)) {
  return false;
 }
 return extractProgramBinary(out);
}

bool ShaderProgram::loadFromBinary(const ProgramBinaryBlob& binary) {
 GLCore::ensureLoaded();
 destroy();
 lastError_.clear();
 if(!GLCore::shaderSupported) {
  lastError_ = "shader entry points unavailable";
  return false;
 }
 if(binary.binaryFormat == 0 || binary.bytes.empty()) {
  lastError_ = "empty program binary";
  return false;
 }
 auto programBinary = binaryProcs().programBinary;
 if(programBinary == nullptr) {
  lastError_ = "glProgramBinary unavailable";
  return false;
 }
 const unsigned int program = GLCore::createProgram();
 if(program == 0) {
  lastError_ = "glCreateProgram returned 0";
  return false;
 }
 programBinary(program, binary.binaryFormat, binary.bytes.data(), static_cast<int>(binary.bytes.size()));
 int success = 0;
 GLCore::getProgramiv(program, kLinkStatus, &success);
 if(success == 0) {
  char log[2048]{};
  GLCore::getProgramInfoLog(program, sizeof(log), nullptr, log);
  lastError_ = log;
  GLCore::deleteProgram(program);
  return false;
 }
 program_ = program;
 uniformCache_.clear();
 samplerKinds_.clear();
 samplerNames_.clear();
 tessellation_ = binary.tessellation;
 reflectSamplers();
 return true;
}
} // namespace net::minecraft::client::gl
