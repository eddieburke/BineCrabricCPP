#include "net/minecraft/client/gl/ShaderCompileService.hpp"
#include "net/minecraft/client/diagnostics/ClientDiagnostics.hpp"
#include "net/minecraft/client/gl/GLCore.hpp"
#include "net/minecraft/client/render/VertexAbi.hpp"
namespace net::minecraft::client::gl {
namespace diagnostics = net::minecraft::client::diagnostics;

ShaderCompileService& ShaderCompileService::instance() {
 static ShaderCompileService service;
 return service;
}

void ShaderCompileService::setCacheDirectory(std::filesystem::path dir) {
 disk_.setRoot(std::move(dir));
}

std::uint64_t ShaderCompileService::submit(ShaderCompileRequest request) {
 return compileBlocking(std::move(request)).contentHash;
}

ShaderCompileResult ShaderCompileService::compileBlocking(ShaderCompileRequest request) {
 if(request.contentHash == 0) {
  const auto& salt = render::vertex_abi::abiSaltString();
  request.contentHash = request.compute
                            ? ShaderProgram::contentHash(true, request.preamble, request.vertex, {}, {}, {}, {}, salt)
                            : ShaderProgram::contentHash(false, request.preamble, request.vertex, request.fragment,
                                                         request.geometry, request.tessControl, request.tessEvaluation, salt);
 }
 return runJobOnCurrentContext(request);
}

void ShaderCompileService::invalidateDiskEntry(std::uint64_t contentHash) {
 disk_.remove(contentHash);
}

void ShaderCompileService::storeDiskEntry(const ProgramBinaryBlob& blob) {
 disk_.store(blob);
}

ShaderCompileResult ShaderCompileService::runJobOnCurrentContext(const ShaderCompileRequest& request) {
 ShaderCompileResult result;
 result.contentHash = request.contentHash;
 if(auto cached = disk_.tryLoad(request.contentHash)) {
  result.ok = true;
  result.fromDisk = true;
  result.binary = std::move(*cached);
  diagnostics::recordWorkSpan("shader.disk.hit", 0);
  return result;
 }
 diagnostics::recordWorkSpan("shader.disk.miss", 0);
 GLCore::ensureLoaded();
 ShaderProgram program;
 ProgramBinaryBlob blob;
 diagnostics::WorkSpan compileSpan("shader.compile");
 const bool compiled =
     request.compute ? program.compileComputeToBinary(blob, request.vertex, request.preamble)
                     : program.compileToBinary(blob, request.vertex, request.fragment, request.preamble,
                                               request.geometry, request.tessControl, request.tessEvaluation);
 if(!compiled) {
  result.ok = false;
  result.binaryUnsupported = program.valid();
  result.error = program.lastError().empty() ? std::string("compile failed") : program.lastError();
  return result;
 }
 blob.contentHash = request.contentHash;
 result.ok = true;
 result.fromDisk = false;
 result.binary = blob;
 disk_.storeAsync(std::move(blob));
 return result;
}
} // namespace net::minecraft::client::gl
