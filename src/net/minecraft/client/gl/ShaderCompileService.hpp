#pragma once
#include <cstdint>
#include <filesystem>
#include <string>
#include "net/minecraft/client/gl/ShaderBinaryCache.hpp"
#include "net/minecraft/client/gl/ShaderProgram.hpp"
namespace net::minecraft::client::gl {
struct ShaderCompileRequest {
 std::uint64_t contentHash = 0;
 bool compute = false;
 std::string preamble;
 std::string vertex;   // or compute source when compute=true
 std::string fragment;
 std::string geometry;
 std::string tessControl;
 std::string tessEvaluation;
};

struct ShaderCompileResult {
 std::uint64_t contentHash = 0;
 bool ok = false;
 bool fromDisk = false;
 // True when the source compiled but no binary could be produced (the driver
 // lacks glGetProgramBinary). Callers should fall back to a source compile.
 bool binaryUnsupported = false;
 std::string error;
 ProgramBinaryBlob binary;
};

// Single-threaded shader compiler, mirroring Iris: every GL call happens on the
// render thread at controlled points (pack load, prewarm, first use). Iris has no
// worker threads, no second GL context and no glProgramBinary; the previous
// cross-context worker design (wglShareLists into the primary context) deadlocked
// the NVIDIA driver at startup and world load (all threads parked in nvoglv64.dll).
// Driver-level parallelism comes from glMaxShaderCompilerThreadsKHR, latched in
// GLCore::init().
class ShaderCompileService {
 public:
 explicit ShaderCompileService() = default;

 // Names the directory that stores extracted driver program binaries.
 void setCacheDirectory(std::filesystem::path dir);
 [[nodiscard]] const std::filesystem::path& cacheDirectory() const noexcept {
  return disk_.root();
 }

  // Compiles now on the calling thread (a GL context must be current) and returns
  // the result (binary or error).
  ShaderCompileResult compileBlocking(const ShaderCompileRequest& request);
  void invalidateDiskEntry(std::uint64_t contentHash);

 private:
 ShaderCompileResult runJobOnCurrentContext(const ShaderCompileRequest& request);

 ShaderBinaryCache disk_{{}};
};
} // namespace net::minecraft::client::gl
