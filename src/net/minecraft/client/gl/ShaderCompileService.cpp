#include "net/minecraft/client/gl/ShaderCompileService.hpp"
#include <algorithm>
#include <chrono>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include "net/minecraft/client/diagnostics/ClientDiagnostics.hpp"
#include "net/minecraft/client/gl/GLCore.hpp"
#include "net/minecraft/client/render/shaders/SourceProcessor.hpp"
#include "net/minecraft/util/concurrent/ThreadCoordinator.hpp"
namespace net::minecraft::client::gl {
namespace diagnostics = net::minecraft::client::diagnostics;
namespace {
struct WorkerContext {
 HWND hwnd = nullptr;
 HDC dc = nullptr;
 HGLRC ctx = nullptr;
};

// Builds a hidden window and a WGL context sharing `share` on the current
// thread. The pixel format is taken from the primary context so the driver
// treats both as the same format (required for wglShareLists). Returns an
// empty WorkerContext on any failure.
WorkerContext createWorkerContext(HGLRC share, int pixelFormat, const PIXELFORMATDESCRIPTOR& primaryPfd) {
 HWND hwnd = CreateWindowExW(0, L"STATIC", L"shader-compile-worker", WS_POPUP, 0, 0, 1, 1, nullptr, nullptr,
                             GetModuleHandleW(nullptr), nullptr);
 if(hwnd == nullptr) return {};
 HDC dc = GetDC(hwnd);
 if(dc == nullptr) {
  DestroyWindow(hwnd);
  return {};
 }
 PIXELFORMATDESCRIPTOR pfd = primaryPfd;
 if(pixelFormat != 0) {
  if(!SetPixelFormat(dc, pixelFormat, &pfd)) {
   ReleaseDC(hwnd, dc);
   DestroyWindow(hwnd);
   return {};
  }
 } else {
  if(pfd.dwFlags == 0) {
   PIXELFORMATDESCRIPTOR fallback{};
   fallback.nSize = sizeof(PIXELFORMATDESCRIPTOR);
   fallback.nVersion = 1;
   fallback.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
   fallback.iPixelType = PFD_TYPE_RGBA;
   pfd = fallback;
  }
  const int chosen = ChoosePixelFormat(dc, &pfd);
  if(chosen == 0 || !SetPixelFormat(dc, chosen, &pfd)) {
   ReleaseDC(hwnd, dc);
   DestroyWindow(hwnd);
   return {};
  }
 }
 HGLRC ctx = wglCreateContext(dc);
 if(ctx == nullptr) {
  ReleaseDC(hwnd, dc);
  DestroyWindow(hwnd);
  return {};
 }
 if(share != nullptr && !wglShareLists(share, ctx)) {
  wglDeleteContext(ctx);
  ReleaseDC(hwnd, dc);
  DestroyWindow(hwnd);
  return {};
 }
 return {hwnd, dc, ctx};
}

void destroyWorkerContext(WorkerContext& context) {
 if(context.ctx != nullptr) {
  wglMakeCurrent(nullptr, nullptr);
  wglDeleteContext(context.ctx);
 }
 if(context.hwnd != nullptr && context.dc != nullptr) ReleaseDC(context.hwnd, context.dc);
 if(context.hwnd != nullptr) DestroyWindow(context.hwnd);
 context = {};
}

std::uint64_t hashRequest(const ShaderCompileRequest& request) {
 if(request.contentHash != 0) return request.contentHash;
 if(request.compute) {
  return ShaderProgram::contentHash(true, request.preamble, request.vertex);
 }
 return ShaderProgram::contentHash(false, request.preamble, request.vertex, request.fragment,
                                   request.geometry, request.tessControl, request.tessEvaluation);
}
} // namespace

struct ShaderCompileService::ContextSpec {
 HGLRC share = nullptr;
 int pixelFormat = 0;
 PIXELFORMATDESCRIPTOR pfd{};
};

ShaderCompileService& ShaderCompileService::instance() {
 static ShaderCompileService service;
 return service;
}

void ShaderCompileService::setCacheDirectory(std::filesystem::path dir) {
 std::lock_guard lock(mutex_);
 disk_.setRoot(std::move(dir));
}

void ShaderCompileService::start() {
 std::lock_guard lock(mutex_);
 if(started_) return;
 stop_ = false;
 render::captureGlShaderSnapshot();
 const unsigned count = std::min(2u, net::minecraft::util::concurrent::ThreadCoordinator::instance().budget().glCompile);
 if(count == 0) return;
 // Snapshot the primary context here (it is current on the main thread) so the
 // worker threads can reproduce its pixel format and share into it. Window and
 // context creation itself happens on the worker threads to keep this call
 // non-blocking; GLFW window creation is main-thread-only and stalls the frame.
 contextSpec_ = std::make_unique<ContextSpec>();
 if(HDC dc = wglGetCurrentDC(); dc != nullptr) {
  contextSpec_->share = wglGetCurrentContext();
  contextSpec_->pixelFormat = GetPixelFormat(dc);
  if(contextSpec_->pixelFormat != 0) {
   DescribePixelFormat(dc, contextSpec_->pixelFormat, sizeof(PIXELFORMATDESCRIPTOR), &contextSpec_->pfd);
  }
 }
 workers_.reserve(count);
 started_ = true;
 liveWorkers_ = count;
 for(unsigned i = 0; i < count; ++i) {
  workers_.emplace_back([this] { workerMain(); });
 }
}

void ShaderCompileService::stop() {
 {
  std::lock_guard lock(mutex_);
  stop_ = true;
  started_ = false;
 }
 queueCv_.notify_all();
 {
  // Bounded wind-down. Workers exit as soon as stop_ is visible (they do not
  // drain the queue), but if one is wedged inside a driver call, detach it
  // instead of hanging shutdown forever on an unbounded join().
  std::unique_lock lock(mutex_);
  if(!queueCv_.wait_for(lock, std::chrono::seconds(3), [&] { return liveWorkers_ == 0; })) {
   for(std::thread& worker : workers_) {
    if(worker.joinable()) worker.detach();
   }
   workers_.clear();
  }
 }
 for(std::thread& worker : workers_) {
  if(worker.joinable()) worker.join();
 }
 workers_.clear();
 {
  std::lock_guard lock(mutex_);
  jobs_.clear();
  while(!queue_.empty()) queue_.pop();
  completed_.clear();
  stop_ = false;
  liveWorkers_ = 0;
 }
}

bool ShaderCompileService::started() const noexcept {
 const std::lock_guard lock(mutex_);
 return started_;
}

std::shared_ptr<ShaderCompileService::Job> ShaderCompileService::findOrCreateJob(ShaderCompileRequest request) {
 request.contentHash = hashRequest(request);
 auto found = jobs_.find(request.contentHash);
 if(found != jobs_.end()) {
  ++found->second->waiters;
  return found->second;
 }
 auto job = std::make_shared<Job>();
 job->request = std::move(request);
 job->waiters = 1;
 jobs_.emplace(job->request.contentHash, job);
 queue_.push(job);
 queueCv_.notify_one();
 return job;
}

std::uint64_t ShaderCompileService::submit(ShaderCompileRequest request) {
 request.contentHash = hashRequest(request);
 std::lock_guard lock(mutex_);
 if(!started_) {
  // No worker can ever complete this job (service not running or every worker
  // failed to come online). Mark it done with binaryUnsupported so async
  // pollers resolve the pending key with a main-thread source compile; leaving
  // it unsubmitted stranded the key in ProgramCache::pending_ forever
  // (hasPending() never went false and the pack never activated).
  auto job = std::make_shared<Job>();
  job->request = std::move(request);
  job->waiters = 1;
  job->result.contentHash = job->request.contentHash;
  job->result.ok = false;
  job->result.binaryUnsupported = true;
  job->result.error = "shader compile service not running";
  job->done = true;
  const std::uint64_t hash = job->request.contentHash;
  completed_.push_back(std::move(job));
  return hash;
 }
 return findOrCreateJob(std::move(request))->request.contentHash;
}

std::vector<std::shared_ptr<ShaderCompileService::Job>> ShaderCompileService::peekCompleted() {
 std::lock_guard lock(mutex_);
 return completed_;
}

void ShaderCompileService::releaseJob(std::uint64_t contentHash) {
 std::lock_guard lock(mutex_);
 // In-flight: the worker will still run, but drops the job from completed_ when
 // no waiter is left (see workerMain).
 if(auto found = jobs_.find(contentHash); found != jobs_.end()) {
  if(found->second->waiters > 0) {
   --found->second->waiters;
  }
  return;
 }
 for(auto it = completed_.begin(); it != completed_.end(); ++it) {
  if((*it)->request.contentHash != contentHash) {
   continue;
  }
  if(--(*it)->waiters == 0) {
   completed_.erase(it);
  }
  return;
 }
}

ShaderCompileResult ShaderCompileService::compileBlocking(ShaderCompileRequest request) {
 request.contentHash = hashRequest(request);
 // Fast path: disk hit without needing a worker.
 if(auto cached = disk_.tryLoad(request.contentHash)) {
  ShaderCompileResult result;
  result.contentHash = request.contentHash;
  result.ok = true;
  result.fromDisk = true;
  result.binary = std::move(*cached);
  diagnostics::recordWorkSpan("shader.disk.hit", 0);
  return result;
 }
 bool haveWorker = false;
 {
  std::lock_guard lock(mutex_);
  haveWorker = started_;
 }
 if(!haveWorker) {
  // No worker available (headless/tests, or every worker failed to come
  // online). Compile on the caller's context, but never while holding the
  // service mutex: a synchronous compile plus disk write must not block every
  // other user of the service.
  return runJobOnCurrentContext(request);
 }
 std::shared_ptr<Job> job;
 {
  std::lock_guard lock(mutex_);
  job = findOrCreateJob(std::move(request));
 }
 std::unique_lock lock(mutex_);
 job->cv.wait(lock, [&] { return job->done || stop_ || !started_; });
 if(!job->done) {
  if(!started_) {
   // Every worker failed to come online; compile on the caller's context.
   lock.unlock();
   releaseJob(job->request.contentHash);
   return runJobOnCurrentContext(job->request);
  }
  ShaderCompileResult failed;
  failed.contentHash = job->request.contentHash;
  failed.error = "compile service stopped";
  return failed;
 }
 lock.unlock();
 releaseJob(job->request.contentHash);
 return job->result;
}

void ShaderCompileService::invalidateDiskEntry(std::uint64_t contentHash) {
 std::lock_guard lock(mutex_);
 disk_.remove(contentHash);
 jobs_.erase(contentHash);
}

void ShaderCompileService::storeDiskEntry(const ProgramBinaryBlob& blob) {
 std::lock_guard lock(mutex_);
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
  // The source compiled but extracting a GL program binary failed (driver lacks
  // glGetProgramBinary / returned an empty binary). The program handle survives a
  // failed extraction, so valid() distinguishes this from a genuine shader error
  // and lets the caller fall back to a plain source compile.
  result.binaryUnsupported = program.valid();
  result.error = program.lastError().empty() ? std::string("compile failed") : program.lastError();
  return result;
 }
 blob.contentHash = request.contentHash;
 result.ok = true;
 result.fromDisk = false;
 result.binary = std::move(blob);
 disk_.store(result.binary);
 return result;
}

void ShaderCompileService::workerMain() {
 ContextSpec* spec = contextSpec_.get();
 WorkerContext context = createWorkerContext(spec->share, spec->pixelFormat, spec->pfd);
 if(context.ctx != nullptr && !wglMakeCurrent(context.dc, context.ctx)) {
  // CRITICAL: every GL call below must run with this thread's context current,
  // or the driver wedges the whole process (all threads parked in nvoglv64.dll
  // on NVIDIA). The WGL rewrite dropped the GLFW make-current step; treat a
  // failed make-current exactly like a failed context creation.
  destroyWorkerContext(context);
 }
 if(context.ctx == nullptr) {
  std::vector<std::shared_ptr<Job>> stranded;
  {
   std::lock_guard lock(mutex_);
   if(liveWorkers_ > 0) --liveWorkers_;
   if(liveWorkers_ == 0) {
    // No worker can serve jobs; wake blocked compileBlocking callers so they
    // fall back to compiling on their own context.
    started_ = false;
    for(auto& entry : jobs_) entry.second->cv.notify_all();
    // The async path has no such wakeup: ProgramCache::markPending only checks
    // started() at SUBMIT time, and start() is deliberately non-blocking, so a key
    // enqueued before the workers failed their context creation stayed in
    // ProgramCache::pending_ with nothing left to complete it — hasPending() never
    // went false and shader compilation hung forever.
    // Hand those jobs back as "compiled, but no binary": that is what
    // binaryUnsupported already means, and ProgramCache::poll() answers it by
    // compiling job->request on the main thread, which is the same fallback
    // compileBlocking callers get.
    while(!queue_.empty()) {
     std::shared_ptr<Job> job = queue_.front();
     queue_.pop();
     if(job == nullptr) continue;
     job->result = ShaderCompileResult{};
     job->result.contentHash = job->request.contentHash;
     job->result.ok = false;
     job->result.binaryUnsupported = true;
     job->result.error = "no shader compile worker context; compiled on the main thread";
     job->done = true;
     jobs_.erase(job->request.contentHash);
     if(job->waiters > 0) {
      completed_.push_back(job);
     }
     stranded.push_back(std::move(job));
    }
   }
   queueCv_.notify_all();
  }
  for(const std::shared_ptr<Job>& job : stranded) job->cv.notify_all();
  return;
 }
 GLCore::ensureLoaded();
 for(;;) {
  std::shared_ptr<Job> job;
  bool dropped = false;
  {
   std::unique_lock lock(mutex_);
   queueCv_.wait(lock, [&] { return stop_ || !queue_.empty(); });
   // Wind down without draining the queue; stop() discards what is left.
   if(stop_) break;
   job = queue_.front();
   queue_.pop();
   if(job->waiters == 0) {
    // Lost-wakeup guard: releaseJob (e.g. ProgramCache::releasePending) can
    // race a compileBlocking waiter still blocked on job->cv. Mark the job done
    // so that waiter wakes instead of waiting forever on a job that never
    // completes.
    jobs_.erase(job->request.contentHash);
    job->result = ShaderCompileResult{};
    job->result.contentHash = job->request.contentHash;
    job->done = true;
    dropped = true;
   }
  }
  if(dropped) {
   job->cv.notify_all();
   continue;
  }
  ShaderCompileResult result = runJobOnCurrentContext(job->request);
  {
   std::lock_guard lock(mutex_);
   job->result = std::move(result);
   job->done = true;
   jobs_.erase(job->request.contentHash);
   if(job->waiters > 0) {
    completed_.push_back(job);
   }
  }
  job->cv.notify_all();
 }
 destroyWorkerContext(context);
 {
  std::lock_guard lock(mutex_);
  if(liveWorkers_ > 0) --liveWorkers_;
  if(liveWorkers_ == 0) started_ = false;
  queueCv_.notify_all();
 }
}
} // namespace net::minecraft::client::gl
