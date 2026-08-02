#include "net/minecraft/client/gl/ProgramCache.hpp"
#include "net/minecraft/client/gl/ShaderCompileService.hpp"
#include <utility>
#include <vector>
namespace net::minecraft::client::gl {
namespace {
// Where did the disk-backed binary path land for this request?
enum class CacheBinaryOutcome {
  Loaded,        // program now holds a usable GL program (fast path or worker compile)
  Failed,        // genuine compile/link error; result.error is the info log
  Unsupported,   // binary extraction/load unavailable — callers compile from source
};
// Runs the request through the ShaderCompileService (disk-cache hit, or worker
// compile + disk store on miss) and, when a binary comes back, loads it into
// `program`. A stale driver binary that the current context rejects is dropped
// from the disk cache so the next launch recompiles it. Only used on the
// synchronous fallback path (service not running: tests/headless).
CacheBinaryOutcome loadBinary(ShaderProgram& program,
                              ShaderCompileRequest request,
                              ShaderCompileResult& resultOut) {
 ShaderCompileResult result = ShaderCompileService::instance().compileBlocking(std::move(request));
 resultOut = std::move(result);
 if(!resultOut.ok) {
  return resultOut.binaryUnsupported ? CacheBinaryOutcome::Unsupported : CacheBinaryOutcome::Failed;
 }
 if(program.loadFromBinary(resultOut.binary)) {
  return CacheBinaryOutcome::Loaded;
 }
 ShaderCompileService::instance().invalidateDiskEntry(resultOut.contentHash);
 return CacheBinaryOutcome::Unsupported;
}
ShaderCompileRequest buildRequest(bool compute,
                                  const std::string& vertexSource,
                                  const std::string& fragmentSource,
                                  const std::string& versionPreamble,
                                  const std::string& geometrySource,
                                  const std::string& tessControlSource,
                                  const std::string& tessEvaluationSource) {
 ShaderCompileRequest request;
 request.compute = compute;
 request.preamble = versionPreamble;
 request.vertex = vertexSource;
 request.fragment = fragmentSource;
 request.geometry = geometrySource;
 request.tessControl = tessControlSource;
 request.tessEvaluation = tessEvaluationSource;
 request.contentHash = ShaderProgram::contentHash(compute, versionPreamble, vertexSource, fragmentSource,
                                                  geometrySource, tessControlSource, tessEvaluationSource);
 return request;
}
} // namespace

ProgramCache::~ProgramCache() {
 releasePending();
}

std::uint64_t ProgramCache::markPending(const std::string& key, ShaderCompileRequest request) {
 if(cache_.contains(key)) {
  return 0;
 }
 if(auto found = pending_.find(key); found != pending_.end()) {
  return found->second;
 }
 ShaderCompileService& service = ShaderCompileService::instance();
 if(!service.started()) {
  // Nothing will complete in the background; the synchronous path handles this key.
  return request.contentHash;
 }
 pending_.emplace(key, request.contentHash);
 service.submit(std::move(request));
 return request.contentHash;
}

ShaderProgram* ProgramCache::getFromSource(const std::string& key,
                                           const std::string& vertexSource,
                                           const std::string& fragmentSource,
                                           const std::string& versionPreamble,
                                           const std::string& geometrySource,
                                           const std::string& tessControlSource,
                                           const std::string& tessEvaluationSource) {
 if(const auto found = cache_.find(key); found != cache_.end()) {
  return found->second.failed ? nullptr : found->second.program.get();
 }
 ShaderCompileRequest request =
     buildRequest(false, vertexSource, fragmentSource, versionPreamble, geometrySource, tessControlSource,
                  tessEvaluationSource);
 if(ShaderCompileService::instance().started()) {
  markPending(key, std::move(request));
  return nullptr; // worker compile in flight; poll() links it next frame
 }
 return getFromSourceBlocking(key, std::move(request), false);
}

ShaderProgram* ProgramCache::getFromComputeSource(const std::string& key,
                                                  const std::string& computeSource,
                                                  const std::string& versionPreamble) {
 if(const auto found = cache_.find(key); found != cache_.end()) {
  return found->second.failed ? nullptr : found->second.program.get();
 }
 ShaderCompileRequest request = buildRequest(true, computeSource, {}, versionPreamble, {}, {}, {});
 if(ShaderCompileService::instance().started()) {
  markPending(key, std::move(request));
  return nullptr;
 }
 return getFromSourceBlocking(key, std::move(request), true);
}

std::uint64_t ProgramCache::submit(const std::string& key,
                                   const std::string& vertexSource,
                                   const std::string& fragmentSource,
                                   const std::string& versionPreamble,
                                   const std::string& geometrySource,
                                   const std::string& tessControlSource,
                                   const std::string& tessEvaluationSource) {
 ShaderCompileRequest request =
     buildRequest(false, vertexSource, fragmentSource, versionPreamble, geometrySource, tessControlSource,
                  tessEvaluationSource);
 return markPending(key, std::move(request));
}

std::uint64_t ProgramCache::submitCompute(const std::string& key,
                                          const std::string& computeSource,
                                          const std::string& versionPreamble) {
 ShaderCompileRequest request = buildRequest(true, computeSource, {}, versionPreamble, {}, {}, {});
 return markPending(key, std::move(request));
}

bool ProgramCache::isPending(const std::string& key) const {
 return pending_.contains(key);
}

bool ProgramCache::poll() {
 auto& service = ShaderCompileService::instance();
 if(pending_.empty()) {
  return false;
 }
 std::vector<std::shared_ptr<ShaderCompileService::Job>> jobs = service.peekCompleted();
 if(jobs.empty()) {
  return false;
 }
 std::unordered_map<std::uint64_t, std::shared_ptr<ShaderCompileService::Job>> byHash;
 for(const std::shared_ptr<ShaderCompileService::Job>& job : jobs) {
  byHash.emplace(job->request.contentHash, job);
 }
 bool any = false;
 for(auto pending = pending_.begin(); pending != pending_.end();) {
  const std::uint64_t hash = pending->second;
  const auto match = byHash.find(hash);
  if(match == byHash.end()) {
   ++pending;
   continue;
  }
  const std::string key = pending->first;
  const std::shared_ptr<ShaderCompileService::Job>& job = match->second;
  pending = pending_.erase(pending);
  // Link the worker's binary (or compile from source if the driver rejected it).
  Entry entry;
  entry.program = std::make_unique<ShaderProgram>();
  const ShaderCompileResult& result = job->result;
  bool compiled = false;
  if(result.ok) {
   if(!result.binary.bytes.empty()) {
    compiled = entry.program->loadFromBinary(result.binary);
    if(!compiled) {
     // Stale driver binary rejected by this context — drop it and fall back.
     service.invalidateDiskEntry(hash);
    }
   }
   if(!compiled) {
    compiled = job->request.compute
                   ? entry.program->compileCompute(job->request.vertex, job->request.preamble)
                   : entry.program->compile(job->request.vertex, job->request.fragment,
                                            job->request.preamble, job->request.geometry,
                                            job->request.tessControl, job->request.tessEvaluation);
   }
  } else if(result.binaryUnsupported) {
   // Worker compiled the source fine but the driver cannot extract a binary;
   // compile on the main thread (the job->request carries the prepared source).
   compiled = job->request.compute
                  ? entry.program->compileCompute(job->request.vertex, job->request.preamble)
                  : entry.program->compile(job->request.vertex, job->request.fragment,
                                           job->request.preamble, job->request.geometry,
                                           job->request.tessControl, job->request.tessEvaluation);
  }
  if(compiled) {
   cache_.emplace(std::move(key), std::move(entry));
  } else {
   entry.error = result.error.empty() ? entry.program->lastError() : result.error;
   entry.program.reset();
   entry.failed = true;
   cache_.emplace(std::move(key), std::move(entry));
  }
  any = true;
  service.releaseJob(hash);
 }
 return any;
}

ShaderProgram* ProgramCache::getFromSourceBlocking(const std::string& key,
                                                   ShaderCompileRequest request,
                                                   bool compute) {
 Entry entry;
 entry.program = std::make_unique<ShaderProgram>();
 ShaderCompileResult result;
 switch(loadBinary(*entry.program, request, result)) {
 case CacheBinaryOutcome::Loaded:
  cache_.emplace(key, std::move(entry));
  return cache_.find(key)->second.program.get();
 case CacheBinaryOutcome::Failed:
  entry.error = result.error;
  entry.program.reset();
  entry.failed = true;
  cache_.emplace(key, std::move(entry));
  return nullptr;
 case CacheBinaryOutcome::Unsupported:
  break;
 }
 const bool compiled =
     compute ? entry.program->compileCompute(request.vertex, request.preamble)
             : entry.program->compile(request.vertex, request.fragment, request.preamble, request.geometry,
                                      request.tessControl, request.tessEvaluation);
 if(compiled) {
  ShaderProgram* raw = entry.program.get();
  cache_.emplace(key, std::move(entry));
  return raw;
 }
 entry.error = entry.program->lastError();
 entry.program.reset();
 entry.failed = true;
 cache_.emplace(key, std::move(entry));
 return nullptr;
}

const std::string& ProgramCache::compileError(const std::string& key) const {
 static const std::string empty;
 const auto found = cache_.find(key);
 return found == cache_.end() ? empty : found->second.error;
}

void ProgramCache::clear() {
 cache_.clear();
 releasePending();
}

void ProgramCache::cancelPending() {
 releasePending();
}

void ProgramCache::releasePending() {
 if(pending_.empty()) {
  return;
 }
 ShaderCompileService& service = ShaderCompileService::instance();
 for(const auto& [key, hash] : pending_) {
  service.releaseJob(hash);
 }
 pending_.clear();
}
} // namespace net::minecraft::client::gl
